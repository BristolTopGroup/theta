#!/usr/bin/python
# coding=utf8

import matplotlib
# note: png with cairo is broken on ubuntu 10.10: segfaults sometimes in figure.save ...
#matplotlib.use('Cairo')

import matplotlib.pyplot as plt
import matplotlib.font_manager as fm
import matplotlib.text
import matplotlib.lines
import matplotlib.patches
import numpy

import scipy.special
import scipy.optimize as optimize
from scipy import interpolate

import sqlite3, math, os, os.path, pickle, copy, array

import glob

# return a list of result rows for the given query on the .db filename.
def sql_singlefile(filename, query):
    if not os.path.exists(filename): raise RuntimeError, "sql: the file %s does not exist!" % filename
    conn = sqlite3.connect(filename)
    c = conn.cursor()
    try:  c.execute(query)
    except Exception, ex:
        print "exception executing %s on file %s: %s" % (query, filename, str(ex))
        raise ex
    result = c.fetchall()
    c.close()
    conn.close()
    return result

def sql(filename_pattern, query):
    result = []
    for f in glob.glob(filename_pattern):
        result.extend(sql_singlefile(f, query))
    return result
                            
def add_xlabel(axes, text, *args, **kwargs):
    label = axes.set_xlabel(text, size='large', ha='right', *args, **kwargs)
    label.set_position((1.0, 0.03))
    return label

def add_ylabel(axes, text, *args, **kwargs):
    label = axes.set_ylabel(text, size='large', va='top', *args, **kwargs)
    label.set_position((-0.03, 1.0))
    return label

#add secondary title:
def add_stitle(ax, title):
    return ax.text(0.0, 1.02, title, transform = ax.transAxes, ha='left', va='bottom')

class plotdata:
    def __init__(self):
        self.x = []
        self.y = []
        self.legend = None
        self.yerrors = None
        self.xerrors = None
        self.fill_color = None
        self.color = None
        self.marker = 'None'
        self.lw = 2
        # an array of bands; a band is a three-tuple (y1, y2, color). y1 and y2 are 
        # arrays of y values.
        # bands to draw first should come first 
        self.bands = None
        self.band_lw = 0
        self.bands_fill = True
        self.as_function = False

#    def set_from_nphisto(self, histo):
#        self.x = histo[1]
#        self.y = histo[0]
#        #shift x values:
#        self.xerrors = [0.5 * (x2 - x1) for x1, x2 in zip(self.x[:-1], self.x[1:])]
#        self.x = [0.5 * (x1 + x2) for x1, x2 in zip(self.x[:-1], self.x[1:])]
#        self.yerrors = map(math.sqrt, self.y)

#histos is a list of tuples (label, histogram)
def plot(histos, xlabel, ylabel, outname, logy = False, logx = False, legend_args = {}, ax_modifier=None, stitle=None,
 xmin = None, xmax=None, ymin=None, ymax=None):
    cm = 1.0/2.54
    fsize = 15*cm, 12*cm
    fp = fm.FontProperties(size=10)
    fig = plt.figure(figsize = fsize)
    rect = 0.15, 0.15, 0.8, 0.75
    ax = fig.add_axes(rect)
    if logy:  ax.set_yscale('log')
    if logx: ax.set_xscale('log')
    add_xlabel(ax, xlabel, fontproperties=fp)
    add_ylabel(ax, ylabel, fontproperties=fp)
    if stitle is not None: add_stitle(ax, stitle)
    draw_legend = False
    for histo in histos:
        assert len(histo.x)==len(histo.y)
        if histo.legend: draw_legend = True
        if histo.bands is not None:
            for band in histo.bands:
                if histo.bands_fill:
                    ax.fill_between(histo.x, band[0], band[1], lw=histo.band_lw, facecolor=band[2], color=band[2])
                else:
                    xs = histo.x + [x for x in reversed(histo.x)]
                    ys = band[0] + [y for y in reversed(band[1])]
                    xs.append(xs[0])
                    ys.append(ys[0])
                    ax.plot(xs, ys, lw=histo.band_lw, color=band[2])
        if not histo.as_function:
            # histo.x is assumed to contain the lower bin edges in this case ...
            x_binwidth = histo.x[1] - histo.x[0]
            # if histo.yerrors is set, draw with errorbars, shifted by 1/2 binwidth ...
            if histo.yerrors is not None:
               new_x = [x + 0.5 * x_binwidth for x in histo.x]
               ax.errorbar(new_x, histo.y, histo.yerrors, label=histo.legend, ecolor = histo.color, marker='o', ms = 2, capsize = 0, fmt = None)
            else:
               new_x = [histo.x[0]]
               for x in histo.x[1:]: new_x += [x]*2
               new_x += [histo.x[-1] + x_binwidth]
               new_y = []
               for y in histo.y: new_y += [y]*2
               if histo.fill_color is not None:
                   ax.fill_between(new_x, new_y, [0] * len(new_y), lw=histo.lw, label=histo.legend, color=histo.color, facecolor = histo.fill_color)
               else:
                   ax.plot(new_x, new_y, lw=histo.lw, label=histo.legend, color=histo.color)
               #ax.bar(map(lambda x: x - 0.5  * x_binwidth, histo.x), histo.y, width=x_binwidth, color=histo.color, label=histo.legend, ec=histo.color)
        else: ax.plot(histo.x, histo.y, lw=histo.lw, label=histo.legend, color=histo.color, marker=histo.marker)

    if draw_legend: ax.legend(prop=fp,**legend_args)
    if ax.get_legend() is not None:
        map(lambda line: line.set_lw(1.5), ax.get_legend().get_lines())

    if ymin!=None:
        ax.set_ylim(ymin=ymin)
    if ymax!=None:
        ax.set_ylim(ymax=ymax)
    if xmin!=None:
        ax.set_xlim(xmin=xmin)
    if xmax!=None:
        ax.set_xlim(xmax=xmax)
    
    if ax_modifier!=None: ax_modifier(ax)
    fig.savefig(outname)
    del fig

def make_stack(pdatas):
    for i in range(len(pdatas)):
        for j in range(i+1, len(pdatas)):
            pdatas[i].y = map(lambda x: x[0] + x[1], zip(pdatas[i].y, pdatas[j].y))


def p_to_Z(p):
    if p==1.0: return 0.0
    return math.sqrt(2) * scipy.special.erfinv(1 - 2*p)

def plotdata_from_histoColumn(h):
    a = array.array('d')
    a.fromstring(h)
    pdata = plotdata()
    xmin = a[0]
    xmax = a[1]
    nbins = len(a) - 4
    binwidth = (xmax - xmin) / nbins
    pdata.x = [xmin + i*binwidth for i in range(nbins)]
    pdata.y = a[3:-1]
    return pdata
    
def findmax(pd):
   m = -float("inf")
   mpos = -1
   for i in range(len(pd.x)):
       if pd.y[i] > m:
           m = pd.y[i]
           mpos = i
   return pd.x[mpos]
   
def central_1sigma(pd):
   m = -float("inf")
   mpos = -1
   for i in range(len(pd.x)):
       if pd.y[i] > m:
           m = pd.y[i]
           mpos = i
   target = 0.6827 * sum(pd.y)
   s = 0
   mpos_low = mpos
   mpos_high = mpos
   # interval is (mpos_low, mpos_high]
   while s < target:
       if mpos_low >= 0 and mpos_high < len(pd.y) - 1:
           if pd.y[mpos_low] > pd.y[mpos_high+1]:
               s += pd.y[mpos_low]
               mpos_low -= 1
           else:
               mpos_high += 1
               s += pd.y[mpos_high]
       elif mpos_low >= 0:
           s += pd.y[mpos_low]
           mpos_low -= 1
       else:
           mpos_high += 1
           s += pd.y[mpos_high]
   return pd.x[mpos_low+1], pd.x[mpos_high]
           
           
   
    





# sql_query should select (x,y) values which will be plotted. Make
# sure to sort the x value ...
def plot_sql_xy(fname, sql_query, outname, xlabel = None, ylabel = None):
    data = sql(fname, sql_query)
    pd = plotdata()
    pd.x = [d[0] for d in data]
    pd.y = [d[1] for d in data]
    pd.color = '#0000ff'
    pd.as_function = True
    if xlabel is None:
        xlabel = '$x$'
    if ylabel is None:
        ylabel = '$y$'
    plot((pd,), xlabel, ylabel, outname)

# sql_query should select 1 double. For this double, the histogram is constructed.
# r is the range as tuple (xmin, xmax)
def plot_sql_histogram(fname, sql_query, r, nbins, outname, xlabel = None):
    data = sql(fname, sql_query)
    data = [d[0] for d in data]    
    pd = plotdata()
    binwidth = (r[1] - r[0]) / nbins
    pd.x = [r[0] + binwidth * i for i in range(nbins)]
    pd.y = [0] * nbins
    for d in data:
        ibin = (d - r[0]) / binwidth
        if ibin < 0 or ibin >= nbins: continue
        pd.y[ibin] += 1.0
    if xlabel is None:
        xlabel = '$x$'
    plot((pd,), xlabel, "$N$", outname)


def plot_ordering_vs_ts():
    truth_values = sql('gaussoverflat-ordering.db', 'select distinct(truth) from ordering')
    truth_values = sorted([t[0] for t in truth_values])
    for t in truth_values[0], truth_values[len(truth_values)/2], truth_values[-1]:
        plot_sql_xy('gaussoverflat-ordering.db', 'select ts, ordering from ordering where truth=%.30g order by ts' % t,
          'o_vs_ts__truth_%g.pdf' % t, xlabel = 'ts', ylabel = 'ordering')

#plot_ordering_vs_ts()




def calc_band(mass, channel, data_value = None):
    data = sql('theta_output/zprime%dw1-mle-scan-%s.db' % (mass, channel), 'select source__beta_zprime, mle__beta_zprime from products')
    
    beta_in = numpy.sort(list(set([d[0] for d in data])))
    pdata = plotdata()
    pdata.x = beta_in
    pdata.y = []
    y95 = []
    for beta in beta_in:
        # calculate 5% and 50% quantile of fitted zprime xs:
        beta_fitted = [d[1] for d in data if (d[0]==beta)]
        beta_fitted = numpy.sort(beta_fitted)
        pdata.y.append(beta_fitted[int(len(beta_fitted)/2)])
        y95.append(beta_fitted[int(0.05 * len(beta_fitted))])
        if beta==beta_in[0]:
            beta0_50 = beta_fitted[int(len(beta_fitted)/2)]
            beta0_16 = beta_fitted[int(len(beta_fitted)*0.16)]
            beta0_84 = beta_fitted[int(len(beta_fitted)*0.84)]
            beta0_025 = beta_fitted[int(len(beta_fitted)*0.025)]
            beta0_975 = beta_fitted[int(len(beta_fitted)*0.975)]
    
    # find out where y95 is "non-zero" and fit that linearly:
    y95max = max(y95)
    i = 0
    while i < len(y95) and y95[i] < 0.0001 * y95max: i+=1
    y95_r = y95[i:]
    beta_in_r = beta_in[i:]
    
    p = [1.0, -0.5]
    func = lambda p, x: p[0] * x + p[1]
    fitfunc = lambda p, x: numpy.array(map(lambda xx: func(p, xx), x))
    errfunc = lambda p, x, y: fitfunc(p, x) - y
    p1, success = optimize.leastsq(errfunc, p[:], args=(beta_in_r, y95_r))
    
    ul_16 = optimize.fsolve(lambda x: func(p1,x) - beta0_16, beta_in_r[0])
    ul_50 = optimize.fsolve(lambda x: func(p1,x) - beta0_50, beta_in_r[0])
    ul_84 = optimize.fsolve(lambda x: func(p1,x) - beta0_84, beta_in_r[0])
    ul_025 = optimize.fsolve(lambda x: func(p1,x) - beta0_025, beta_in_r[0])
    ul_975 = optimize.fsolve(lambda x: func(p1,x) - beta0_975, beta_in_r[0])
    ul_observed = None
    if data_value is not None:
        ul_observed = optimize.fsolve(lambda x: func(p1,x) - data_value, ul_50)
    
    pdata.legend = 'linear'
    pdata.color = '#0000ff'
    
    pdata95 = plotdata()
    pdata95.x = beta_in
    pdata95.y = y95
    pdata95.legend = '95%'
    pdata95.color = '#ff0000'
    
    pdata_fit = plotdata()
    pdata_fit.x = beta_in
    pdata_fit.y = func(p1,beta_in)
    pdata_fit.color = '#ffff00'
    pdata_fit.legend = 'fit'
    
    plot((pdata,pdata95,pdata_fit), r"$\sigma_{in} [pb]$", r"$\sigma_{out}$", "results/band-%d-%s.pdf" % (mass, channel))
    return (ul_025, ul_16, ul_50, ul_84, ul_975, ul_observed)
    
def discovery_band(mass, method, channel):
    ts_name = {'lr': 'hypotest__nll_diff', 'mle':'mle__beta_zprime'}[method]
    data_scan = sql('theta_output/zprime%dw1-%s-scan-%s.db' % (mass, method, channel), 'select source__beta_zprime, %s from products' % ts_name)
    beta_in = numpy.sort(list(set([d[0] for d in data_scan])))
    data_zero = sql('theta_output/zprime%dw1-%s-zero-%s.db' % (mass, method, channel), 'select %s from products' % ts_name)
    data_zero = [d[0] for d in data_zero]
    data_data = sql('theta_output/zprime%dw1-%s-data-%s.db' % (mass, method, channel), 'select %s from products' % ts_name)
    data_data = data_data[0][0]
    p_data = len([x for x in data_zero if x >= data_data]) * 1.0 / len(data_zero)
    Z_data = p_to_Z(p_data)
    print "discovery_band: Z-value for data for zprime%dw-%s is %f" % (mass, method, Z_data)
    
    result = plotdata()
    result.x = beta_in
    result.y = []
    result.bands = [[[], [], '#000000']]
    for beta in beta_in:
        beta_fitted = [d[1] for d in data_scan if (d[0]==beta)]
        beta_fitted = numpy.sort(beta_fitted)
        expected_ts, expected_ts16, expected_ts84 = beta_fitted[int(0.5 * len(beta_fitted))], beta_fitted[int(0.16 * len(beta_fitted))], beta_fitted[int(0.84 * len(beta_fitted))]
        n_above, n_above16, n_above84 = map(lambda ts: len([x for x in data_zero if x >= ts]), (expected_ts, expected_ts16, expected_ts84))
        Z, Z_plus, Z_minus = map(lambda n: p_to_Z(1.0 * max(n, 1) / len(data_zero)), (n_above, n_above84, n_above16))
        result.y.append(Z)
        result.bands[0][0].append(Z_minus)
        result.bands[0][1].append(Z_plus)
        print beta, Z
    return result

def calc_band_bayes(mass, channel):
    data = sql('theta_output/zprime%dw1-bayes-zero-%s.db' % (mass, channel), 'select bayes__quant09500 from products')
    data = [d[0] for d in data]
    data = numpy.sort(data)
    n = len(data)
    if n < 10: raise RuntimeError, "too few values in zprime%dw1-bayes-zero-%s.db" % (mass, channel)
    return (data[int(n*0.025)], data[int(n*0.16)], data[int(n*0.5)], data[int(n*0.84)], data[int(n*0.975)])

def get_bayes_data_result(mass, channel):
    data = sql('theta_output/zprime%dw1-bayes-data-%s.db' % (mass, channel), 'select bayes__quant09500 from products')
    if len(data) < 1: raise RuntimeError, 'theta_output/zprime%dw1-bayes-data-%s.db did not contain data' % (mass, channel)
    return data[0][0]
    
def get_pvalue_bayes(mass, channel, data_value):
    count_above = sql('theta_output/zprime%dw1-bayes-zero-%s.db' % (mass, channel), 'select count(*) from products where bayes__quant09500 > %f' % data_value)
    count_all = sql('theta_output/zprime%dw1-bayes-zero-%s.db' % (mass, channel), 'select count(*) from products')
    return count_above[0][0] * 1.0 / count_all[0][0]


def get_data_result(mass, channel):
    data = sql('theta_output/zprime%dw1-mle-data-%s.db' % (mass, channel), 'select mle__beta_zprime, mle__beta_vjets, mle__beta_ttbar from products')
    d = data[0][0]
    print "data fit for zprime = %d: (zprime, vjets, ttbar) = %s" % (mass, str(data[0]))
    return d
    
def get_p_value(mass, channel, beta_zprime_fitted):
    count_all = sql('theta_output/zprime%dw1-mle-zero-%s.db' % (mass, channel), 'select count(*) from products where 1')
    count_above = sql('theta_output/zprime%dw1-mle-zero-%s.db' % (mass, channel), 'select count(*) from products where mle__beta_zprime > %f' % beta_zprime_fitted)
    return count_above[0][0] * 1.0 / count_all[0][0]

def get_data_pvalue(mass, channel, ts):
    data = sql('theta-output/zprime%dw1-mle-data-%s.db' % (mass, channel), 'select %s from products' % ts)
    d = data[0][0]
    count_all = sql('theta-output/zprime%dw1-mle-zero-%s.db' % (mass, channel), 'select count(*) from products where 1')
    count_above = sql('theta-output/zprime%dw1-mle-zero-%s.db' % (mass, channel), 'select count(*) from products where %s > %f' % (ts, d))
    return count_above[0][0] * 1.0 / count_all[0][0]

# fname_pattern is a pattern containing and will be format-substituted with '%' by each element of the substitutions array
def get_corrected_p_value(fname_pattern, substitutions, ts_colname, p_min):
    data = {}
    # critical values for p_min for all masses:
    crit = {}
    n = -1
    maxeventid = 0
    for s in substitutions:
        data_mass = sql(fname_pattern % mass, 'select eventid, %s from products' % ts_colname)
        # make a map eventid -> result:
        data[mass] = {}
        d_sorted = []
        for d in data_mass:
            data[mass][d[0]] = d[1]
            maxeventid = max(maxeventid, d[0])
            d_sorted.append(d[1])
        d_sorted = numpy.sort(d_sorted)
        crit[mass] = d_sorted[int(len(d_sorted) * (1 - p_min))]
        del d_sorted
    counter = 0
    for ievt in range(1, maxeventid+1):
        for mass in masses:
            if not ievt in data[mass]: continue
            if data[mass][ievt] > crit[mass]:
                counter += 1
                break
    return counter * 1.0 / maxeventid


def band_plot(channel, zmasses, do_significance = False, do_data = True):
    pdata = plotdata()
    pdata.x = []
    pdata.y = []
    band1_low = []
    band1_hi = []
    band2_low = []
    band2_hi = []
    p_values = []
    for zmass in zmasses:
        pdata.x.append(float(zmass))
        if do_data: data_res = get_data_result(zmass, channel)
        else: data_res = None
        ul_025, ul_16, ul_50, ul_84, ul_975, ul_observed = calc_band(zmass, channel, data_res)
        if ul_observed is None: ul_observed = 0.0
        print "Neyman ul for %d-%s: %f (expected: %f, %f)" % (zmass, channel, ul_observed, ul_16, ul_84)
        if do_significance:
            p = get_p_value(zmass, channel, data_res)
            p_values.append(p)
            print "p, Z for %s-%s: " % (zmass, channel), p, p_to_Z(p)
        pdata.y.append(ul_observed)
        band1_low.append(ul_16)
        band1_hi.append(ul_84)
        band2_low.append(ul_025)
        band2_hi.append(ul_975)

    if do_significance:
        p_min = min(p_values)
        p_corr = get_corrected_p_value(zmasses, channel, p_min)
        print "p, Z (corrected %s): " % channel, p_corr, p_to_Z(p_corr)

    pdata.color = '#000000'
    pdata.lw = 1
    pdata.band_lw = 1
    pdata.bands = [[band2_low, band2_hi, '#88ff88'], [band1_low, band1_hi, '#00d200']]
    pdata.legend = 'observed 95% C.L. upper limit'
    pdata_dummy = plotdata()
    pdata_dummy.x = []
    pdata_dummy.y = []
    pdata_dummy.lw = 10
    pdata_dummy.color = pdata.bands[0][2]
    pdata_dummy.legend = 'central 95% expected upper limit'
    pdata_dummy2 = plotdata()
    pdata_dummy2.x = []
    pdata_dummy2.y = []
    pdata_dummy2.lw = 10
    pdata_dummy2.color = pdata.bands[1][2]
    pdata_dummy2.legend = 'central 68% expected upper limit'

    pd_xs = topcolor_xs()

    plot((pdata,pdata_dummy,pdata_dummy2), r"$M_{Z^{\prime}} [\mathrm{GeV}/c^2]$", r"$\sigma(pp\rightarrow Z^\prime) \times BR(Z^\prime \rightarrow t\bar t) \mathrm{[pb]}$",
       "results/ul-%s.pdf" % channel, stitle=r'$\mathcal{L}=\mathrm{%s pb}^{-1}, \sqrt{s} = 7\mathrm{ TeV}$' % lumi)
  
    plot((pdata,pdata_dummy,pdata_dummy2), r"$M_{Z^{\prime}} [\mathrm{GeV}/c^2]$", r"$\sigma(pp\rightarrow Z^\prime) \times BR(Z^\prime \rightarrow t\bar t) \mathrm{[pb]}$",
      "results/ul10-%s.pdf" % channel, stitle=r'$\mathcal{L}=\mathrm{%s pb}^{-1}, \sqrt{s} = 7 \mathrm{TeV}$' % lumi, ymax = 10.0, xmin = 1000)
      
    return pdata
      
def band_plot_bayes(channel, zmasses, use_data = True, calc_pvalue = True):
    pdata = plotdata()
    pdata.x = []
    pdata.y = []
    band1_low, band1_hi = [], []
    band2_low, band2_hi = [], []
    p_min = 1.0
    for zmass in zmasses:
        pdata.x.append(float(zmass))
        ul_025, ul_16, ul, ul_84, ul_975 = calc_band_bayes(zmass, channel)
        if use_data:
            ul = get_bayes_data_result(zmass, channel)
            print "Bayes ul for %d-%s: %f  (expected: %f, %f)" % (zmass, channel,  ul, ul_16, ul_84)
            if calc_pvalue:
                p = get_pvalue_bayes(zmass, channel, ul)
                print "p value (Z): %f   (%f)" % (p, p_to_Z(p))
              	p_min = min(p_min, p)
        pdata.y.append(ul)
        band1_low.append(ul_16)
        band1_hi.append(ul_84)
        band2_low.append(ul_025)
        band2_hi.append(ul_975)

    if use_data and calc_pvalue:
        p = get_corrected_p_value('theta_output/zprime%sw1-bayes-zero-hm.db', zmasses, 'bayes__quant09500', p_min)
        print "corrected p value: %f (Z=%f)" % (p, p_to_Z(p))

    pdata.color = '#000000'
    pdata.lw = 1
    pdata.band_lw = 1
    pdata.bands = [[band2_low, band2_hi, '#88ff88'], [band1_low, band1_hi, '#00d200']]
#    pdata.bands = [[band1_low, band1_hi, '#88ff88']]
    pdata.legend = '%s' % ({True: 'observed (toy data)', False: 'median expected'}[use_data])
    pdata_dummy = plotdata()
    pdata_dummy.lw = 10
    pdata_dummy.color = pdata.bands[0][2]
    pdata_dummy.legend = 'central 95% expected    '
    pdata_dummy2 = plotdata()
    pdata_dummy2.lw = 10
    pdata_dummy2.color = pdata.bands[1][2]
    pdata_dummy2.legend = 'central 68% expected   '
    suffix = 'data' if use_data else 'nodata'
    suffix_title = 'toy data' if use_data else ''
    plot((pdata,pdata_dummy,pdata_dummy2), r"$M_{Z^{\prime}} [\mathrm{GeV}/c^2]$", r"$\sigma(pp\rightarrow Z^\prime) \times BR(Z^\prime \rightarrow t\bar t) \mathrm{[pb]}$",
       "ul-%s-bayes-%s.pdf" % (channel, suffix), stitle=r'$\mathcal{L}=\mathrm{%s pb}^{-1}, \sqrt{s} = 7\mathrm{ TeV}$ %s' % (lumi, suffix_title))
  
    plot((pdata,pdata_dummy,pdata_dummy2), r"$M_{Z^{\prime}} [\mathrm{GeV}/c^2]$", r"$\sigma(pp\rightarrow Z^\prime) \times BR(Z^\prime \rightarrow t\bar t) \mathrm{[pb]}$",
      "ul10-%s-bayes-%s.pdf" % (channel, suffix), stitle=r'$\mathcal{L}=\mathrm{%s pb}^{-1}, \sqrt{s} = 7 \mathrm{TeV}$ %s' % (lumi, suffix_title), ymax = 10.0, xmin = 1000)
      
    return pdata





def array_to_pd(ar):
   result = plotdata()
   result.x = [i for i in range(len(ar))]
   result.y = ar[:]
   result.yerrors = map(lambda x: math.sqrt(x), result.y)
   return result

   
def get_bands(channels, zmasses, force_recreate = False):
  bands = {}
  if not os.path.exists("bands.bin") or force_recreate:
    missing_channels = channels
  else:
    f = open('bands.bin', 'r')
    bands = pickle.load(f)
    f.close()
    missing_channels = []
    for c in channels:
      if c not in bands: missing_channels.append(c)
  
  for c in missing_channels:
    bands[c] = band_plot(c, zmasses = zmasses, do_data = True, do_significance=True)

  f = open('bands.bin', 'w')
  pickle.dump(bands, f)
  f.close()
  return bands



def plot_mtt_fit(observable):
   pred = sql('theta_output/zprime0w1-bayes_bestfit-data-hm.db', 'select bayes__%s_mean from products' % observable)
   pred = pd_from_histoColumn(pred[0][0])
   pw = sql('theta_output/zprime0w1-bayes_bestfit-data-hm.db', 'select bayes__%s_width from products' % observable)
   pw = pd_from_histoColumn(pw[0][0])
   pred.yerrors = pw.y[:]
   pred.color = '#ff0000'
   #pred.as_histo = True

   f = numpy.load('templates.npz')
   data = array_to_pd(f[observable + "_DATA"])
   data.color = '#000000'
   pred.x = data.x[:]
   print "DATA", sum(data.y)
   print "pred", sum(pred.y)
   plot((pred, data), r"$M_{t\bar t}$", "$N$", "meanpred-%s.pdf" % observable)

def discovery_plot():
    lr = discovery_band(1000, 'lr', 'lm')
    #mle_disc = discovery_band(1000, 'mle', 'lm')
    lr.color = '#ff0000'
    lr.bands[0][2] = '#ff8888'
    lr.legend = 'likelihood ratio'
    #mle_disc.color = '#00ff00'
    #mle_disc.bands[0][2] = '#88ff88'
    #mle_disc.legend = 'beta_zprime mle fit'
    stitle=r'$\mathcal{L}=\mathrm{%s pb}^{-1}, \sqrt{s} = 7\mathrm{ TeV}$' % lumi
    plot((lr,), "$\sigma_{Z}'$ [pb]", "$Z$ value", "results/discovery1000.pdf", stitle = stitle, legend_args = dict(loc = 'lower right'))
