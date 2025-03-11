extern crate proc_macro;
use std::str::FromStr;

use proc_macro::TokenStream;
use quote::{quote, ToTokens, TokenStreamExt};
use syn::parse::Parser;
use syn::punctuated::Punctuated;
use syn::{parse_macro_input, Meta, TraitItem, TraitItemFn};


#[proc_macro_attribute]
pub fn make_answer(attr: TokenStream, item: TokenStream) -> TokenStream {
    let args = parse_macro_input!(attr with Punctuated::<Meta, syn::Token![,]>::parse_terminated);
    /* 
    for arg in args {
        match arg {
            Meta::List(l) => {
                println!("Metalist: {:?}", l.tokens);
            },
            Meta::Path(p) => {
                println!("path: {:?}", p.get_ident());
            },
            Meta::NameValue(nv) => {
                println!("namevalue: {:?}, {:?}", nv.path.get_ident(), nv.value.into_token_stream());
            },
        }
    }*/
    let mut t = syn::parse_macro_input!(item as syn::ItemTrait);
    let funcs = t.items.iter()
        .filter_map(|t_item| match t_item {
            TraitItem::Fn(t_item_fn) => Some(t_item_fn),
            _ => None,
        })
        .map(|t_fn: &TraitItemFn | {
            let sig = &t_fn.sig;
            quote! {
                #sig {
                    panic!()
                }
            }

        });
    let struct_name = "Compressor".into_token_stream();
    let struct_def = quote! {
        struct #struct_name {

        }
    };
    let impl_begin = quote! {
        impl #struct_name {
    };
    let impl_end = quote! { } };
    
    let mut out = t.to_token_stream();
    out.extend(struct_def);
    out.extend(impl_begin);
    out.extend(funcs);
    out.extend(impl_end);
    out.into()
}