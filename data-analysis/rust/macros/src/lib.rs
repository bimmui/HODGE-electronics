extern crate proc_macro;
use std::str::FromStr;

use cxxbridge_macro::bridge;
use proc_macro::{Span, TokenStream, TokenTree};
use quote::{format_ident, quote, ToTokens, TokenStreamExt};
use syn::parse::{Parse, Parser};
use syn::punctuated::Punctuated;
use syn::token::SelfType;
use syn::{parse_macro_input, Attribute, FnArg, Ident, Meta, ReturnType, Token, TraitItem, TraitItemFn, Type};



fn returns_self(ret: &ReturnType) -> bool {
    let p1_toks: proc_macro::TokenStream = ret.to_token_stream().into();
    p1_toks.into_iter()
        .filter(|tok| match tok {
            TokenTree::Ident(ident) => { ident.to_string() == "Self" },
            _ => false
        })
        .next().is_some()
}

fn replace_self_concrete<'a>(ret: &'a ReturnType, concrete_type: &syn::Ident) -> ReturnType {
    let ret_parser = ReturnType::parse;

    let p1_toks: proc_macro::TokenStream = ret.to_token_stream().into();
    let tokens = p1_toks.into_iter()
        .map(|tok| 
            if let TokenTree::Ident(ref ident) = tok {
                if ident.to_string() == "Self" {
                    TokenTree::Ident(proc_macro::Ident::new(concrete_type.to_string().as_str(), Span::call_site()))
                } else { tok }
            } else {
                tok
        });
        //.map(|tok| tok.into());
    ret_parser.parse(TokenStream::from_iter(tokens)).unwrap()
}

fn replace_self_concrete_boxed<'a>(ret: &'a ReturnType, concrete_type: &syn::Ident) -> ReturnType 
{
    let ret_parser = ReturnType::parse;

    let p1_toks: proc_macro::TokenStream = ret.to_token_stream().into();
    let tokens = p1_toks.into_iter()
        .map(|tok| 
            if let TokenTree::Ident(ref ident) = tok {
                if ident.to_string() == "Self" {
                    Box::new([
                        TokenTree::Ident(proc_macro::Ident::new("Box", Span::call_site())),
                        TokenTree::Punct(proc_macro::Punct::new('<', proc_macro::Spacing::Alone)),
                        TokenTree::Ident(proc_macro::Ident::new(concrete_type.to_string().as_str(), Span::call_site())),
                        TokenTree::Punct(proc_macro::Punct::new('>', proc_macro::Spacing::Alone)),
                    ].into_iter()) as Box<dyn Iterator<Item = TokenTree>>
                } else { Box::new([tok].into_iter()) as Box<dyn Iterator<Item = TokenTree>> }
            } else {
                Box::new([tok].into_iter()) as Box<dyn Iterator<Item = TokenTree>>
        }).flatten();
        //.map(|tok| tok.into());
    ret_parser.parse(TokenStream::from_iter(tokens)).unwrap()
}


#[proc_macro_attribute]
pub fn export_concrete(attr: TokenStream, item: TokenStream) -> TokenStream {
    let args = parse_macro_input!(attr with Punctuated::<Meta, syn::Token![,]>::parse_terminated);
    let mut ident_str = None;
    let mut struct_str = None;
    let mut mod_str = None;

    for arg in args {
        match arg {
            Meta::NameValue(nv) => {
                println!("namevalue: {:?}, {:?}", nv.path.get_ident(), nv.value.to_token_stream());
                let name = nv.path
                    .get_ident()
                    .expect("macro export_concrete: Name argument must have path")
                    .to_string();
                let toks: proc_macro::TokenStream = nv.value.into_token_stream().into();
                let value = toks
                    .into_iter()
                    .next()
                    .map(|tok| match tok {
                        TokenTree::Ident(ident) => ident.to_string(),
                        _ => {
                            println!("tok: {:?}", tok);
                            panic!("expected ident, recieved {:?}", tok);
                        },
                    }).expect("macro export_concrete: value part of namevalue must be single identifier");
                
                match name.to_lowercase().as_str() {
                    "concrete" => { ident_str = Some(value) },
                    "wrapper" => { struct_str = Some(value) },
                    "mod" | "module" => { mod_str = Some(value) },
                    _ => panic!("Unexpected name: {name}")
                }
            },
            _ => {

            }
        }
    }
    let inner_type = Ident::new(
        ident_str.expect("Must provide a name for the concrete type (#[export_concrete(..., concrete = foo, ...)])").as_str(), 
        Span::call_site().into()
    );
    let struct_name = Ident::new(
        struct_str.expect("Must provide a name for the wrapper type (#[export_concrete(..., wrapper = foo_wrapper, ...)])").as_str(), 
        Span::call_site().into()
    );
    let mod_name = Ident::new(
        mod_str.expect("Must provide a name for the module (#[export_concrete(..., mod | module = foo_module, ...)])").as_str(),  
        Span::call_site().into()
    );

    let t = syn::parse_macro_input!(item as syn::ItemTrait);
    let func_defs = t.items.iter()
    .filter_map(|t_item| match t_item {
        TraitItem::Fn(t_item_fn) => Some(t_item_fn),
        _ => None,
    });
    let funcs = func_defs.clone()
        .map(|t_fn: &TraitItemFn | {
            let sig = &t_fn.sig;
            let name = &sig.ident;
            let params = sig.inputs.iter().filter(|param| !matches!(param, FnArg::Receiver(_)))
                .map(|p| p.into_token_stream());
            if sig.receiver().is_some() {
                quote! {
                    #sig {
                        self.inner.#name(#(#params)*)
                    }
                }
            } else if returns_self(&sig.output) {
                quote! {
                    #sig {
                       Self { inner: #inner_type::#name(#(#params)*) }
                    }
                }
            } else {
                quote! {
                    #sig {
                        #inner_type::#name(#(#params)*)
                    }
                }
            }
    });

    let func_defs_noself = func_defs.clone()
        .map(|t_fn| {
            let sig = &t_fn.sig;
            if sig.receiver().is_some() {
                quote! {
                    #sig;
                }
            } else if returns_self(&sig.output) {
                let mut replaced_sig = sig.clone();
                replaced_sig.output = replace_self_concrete_boxed(&sig.output, &struct_name);
                quote! {
                    #replaced_sig;
                }
            } else {
                quote! {
                    #sig;
                }
            }       
        });

    let funcs_to_export = func_defs
        .filter_map(|t_fn| {
            if returns_self(&t_fn.sig.output) {
                Some(t_fn)
            } else { None }
        })
       .map(|t_fn| {
            let sig = &t_fn.sig;
            let name = &sig.ident;
            let params = &sig.inputs;
            let mut replaced_sig = sig.clone();
            replaced_sig.output = replace_self_concrete_boxed(&sig.output, &struct_name);
            quote! {
                #replaced_sig {
                    Box::new(#struct_name::#name(#params))
                }
            }
        });

    quote! { 
        #t 

        struct #struct_name {
            inner: #inner_type,
        }

        impl #struct_name {
            #(#funcs)*
        }

        #(#funcs_to_export)*

        #[cxx::bridge(namespace = "compress")]
        mod #mod_name {
            extern "Rust" {
                type #struct_name;

                #(#func_defs_noself)*
            }
        } 
    }.into()
}