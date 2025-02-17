from tkinter import *
import tkinter
import tkintermapview


""" def button_click():
    print("Button clicked") """

#create app window
root = tkinter.Tk()
root.geometry(f"{800}x{600}")
root.title("Sample tkinter application")

#create button widget
""" button = tkinter.Button(root, text = "Click me", command=button_click)
button.pack(pady=10) """

# create map widget
map_widget = tkintermapview.TkinterMapView(root, width=800, height=600, corner_radius=0)

map_widget.place(relx=.5, rely=.5, anchor=tkinter.CENTER)
map_widget.set_position(42.406,-71.116) #this is Tufts
map_widget.set_zoom(15)


# set a position marker
marker_2 = map_widget.set_marker(42.40640192045009, -71.11896243416697, text="building") #JCC?
#marker_3 = map_widget.set_marker(52.55, 13.4, text="52.55, 13.4")

""" # methods
marker_3.set_position(...)
marker_3.set_text(...)
marker_3.change_icon(new_icon)
marker_3.hide_image(True)  # or False
marker_3.delete() """


def run_periodic_background_func():
   marker_2.set_position(42.40344408336258, -71.11823680988293) #Put update function here
   print("ran after func")
   root.after(2000,run_periodic_background_func)

run_periodic_background_func()
# Run the Tkinter event loop
root.mainloop()
