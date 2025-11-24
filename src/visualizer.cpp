#include "visualizer.h"
#include <cmath>
#include <cstdlib>
#include <gtk/gtk.h>

Visualizer::Visualizer(AppState* state) : app(state) {}

gboolean Visualizer::onTick(gpointer widget) {
    if (GTK_IS_WIDGET(widget)) {
        gtk_widget_queue_draw(GTK_WIDGET(widget));
    }
    return G_SOURCE_CONTINUE;
}

gboolean Visualizer::onDraw(GtkWidget* widget, cairo_t* cr, gpointer data) {
    // FIX: Cast data to Visualizer*, then access its 'app' member
    Visualizer* self = (Visualizer*)data;
    AppState* state = self->app; // <--- Now the private field is USED!
    
    GtkAllocation alloc;
    gtk_widget_get_allocation(widget, &alloc);
    int width = alloc.width;
    int height = alloc.height;

    // 1. Background (Black)
    cairo_set_source_rgb(cr, 0, 0, 0);
    cairo_paint(cr);

    // 2. Clip
    cairo_rectangle(cr, 0, 0, width, height);
    cairo_clip(cr);

    if (!state->playing || state->paused) {
        // Draw flat line
        cairo_set_source_rgb(cr, 0, 0.88, 0); 
        cairo_set_line_width(cr, 2);
        cairo_move_to(cr, 0, height / 2.0);
        cairo_line_to(cr, width, height / 2.0);
        cairo_stroke(cr);
        return FALSE;
    }

    // 3. Draw Fake Visualization
    cairo_set_source_rgb(cr, 0, 0.88, 0); 
    
    int bars = 32;
    double barWidth = (double)width / bars;
    
    for (int i = 0; i < bars; i++) {
        double h = (rand() % (height - 4)) + 4;
        
        double x = i * barWidth + 1;
        double y = height - h;
        
        cairo_rectangle(cr, x, y, barWidth - 2, h);
        cairo_fill(cr);
        
        // Peak
        cairo_set_source_rgb(cr, 0.8, 0.8, 0);
        cairo_rectangle(cr, x, y - 3, barWidth - 2, 2);
        cairo_fill(cr);
        
        cairo_set_source_rgb(cr, 0, 0.88, 0); 
    }

    return FALSE;
}
