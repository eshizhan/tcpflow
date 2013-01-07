/**
 * port_histogram.cpp: 
 * Show packets received vs port
 *
 * This source file is public domain, as it is not based on the original tcpflow.
 *
 * Author: Michael Shick <mike@shick.in>
 *
 */

#include "config.h"
#include "tcpflow.h"
#include "tcpip.h"

#include <algorithm>

#include "port_histogram.h"

void port_histogram::ingest_packet(const packet_info &pi)
{
    struct tcp_seg tcp;

    if(!tcpip::tcp_from_ip_bytes(pi.data, pi.caplen, tcp)) {
        return;
    }

    if(relationship == SENDER || relationship == SND_OR_RCV) {
        port_counts[ntohs(tcp.header->th_sport)]++;
    }
    if(relationship == RECEIVER || relationship == SND_OR_RCV) {
        port_counts[ntohs(tcp.header->th_dport)]++;
    }
}

void port_histogram::render(cairo_t *cr, const plot::bounds_t &bounds)
{
#ifdef CAIRO_PDF_AVAILABLE
    plot::ticks_t ticks;
    plot::legend_t legend;
    plot::bounds_t content_bounds(0.0, 0.0, bounds.width,
            bounds.height);
    //// have the plot class do labeling, axes, legend etc
    parent.render(cr, bounds, ticks, legend, content_bounds);

    //// fill borders rendered by plot class
    render_bars(cr, content_bounds, build_port_list());
#endif
}

void port_histogram::render_bars(cairo_t *cr, const plot::bounds_t &bounds,
        const std::vector<count_pair> &bars)
{
    if(bars.size() < 1 || bars.at(0).second < 1) {
        return;
    }
#ifdef CAIRO_PDF_AVAILABLE
    cairo_matrix_t original_matrix;

    cairo_get_matrix(cr, &original_matrix);
    cairo_translate(cr, bounds.x, bounds.y);

    cairo_set_source_rgb(cr, bar_color.r, bar_color.g, bar_color.b);

    double offset_unit = bounds.width / bars.size();
    double bar_width = offset_unit / bar_space_factor;
    uint64_t greatest = bars.at(0).second;
    int index = 0;
    for(vector<count_pair>::const_iterator port = bars.begin();
	port != bars.end(); port++) {
	double bar_height = (((double) port->second) / ((double) greatest)) * bounds.height;

	if(bar_height > 0) {
	    cairo_rectangle(cr, index * offset_unit, bounds.height - bar_height,
			    bar_width, bar_height);
	    cairo_fill(cr);
	}
	index++;
    }

    cairo_set_matrix(cr, &original_matrix);
#endif
}

std::vector<port_histogram::count_pair> port_histogram::build_port_list()
{
    std::vector<count_pair> output(port_counts.begin(), port_counts.end());

    std::sort(output.begin(), output.end(), count_comparator());

    output.resize(max_bars);

    return output;
}

bool port_histogram::count_comparator::operator()(const count_pair &a,
        const count_pair &b)
{
    if(a.second > b.second) {
        return true;
    }
    if(a.second < b.second) {
        return false;
    }
    return a.first > b.first;
}
