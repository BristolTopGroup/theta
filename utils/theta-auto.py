#!/usr/bin/env python


# TODO:
# commands for:
# * frequentist limit method
# * discovery
# * simple fitting
# * posterior plots for nuisance parameters
# * to make plots better usable, include the option wo write any plot data to a txt file
#
# bugs:
# * if (by appying template filters, etc.), the model does not longer depend on a nuisance parameter, it might still be added to parameter-distribution
#   in some cases.

import os, os.path, ROOT, datetime, re, sys, copy, traceback, numpy, shutil, hashlib

class rootfile:
   def __init__(self, filename):
       self.filename = filename
       self.tfile = ROOT.TFile(filename, "read")

   # returns a list of strings, the histograms (TH1, TH2, TH3) in the root file:
   def get_histograms(self):
       result = []
       l = self.tfile.GetListOfKeys()
       for key in l:
           clas = key.GetClassName()
           if clas not in ('TH1F', 'TH1D'):
               print "WARNING: ignoring key %s in input file %s because it is of ROOT class %s, not TH1F / TH1D" % (key.GetName(), self.filename, clas)
               continue
           result.append(str(key.GetName()))
       return result

   # returns the histogram as tuple (xmin, xmax, data) where data is the data
   # as array of floats, *excluding* overflow, underflow
   def get_histogram(self, histoname):
      th1 = self.tfile.Get(histoname)
      xmin, xmax, nbins = th1.GetXaxis().GetXmin(), th1.GetXaxis().GetXmax(), th1.GetNbinsX()
      data = []
      for i in range(1, nbins+1):
          data.append(th1.GetBinContent(i))
      return xmin, xmax, data



class Distribution:
    def __init__(self):
        # parameters is a dictionary
        # string -> (float, float, (float, float))
        # parameter name -> mean, width, range
        self.parameters = {}
        # correlations is a dictionary
        # (string, string) -> float
        # parameter name 1, parameter name 2 -> correlation coefficient
        # where the names in the key are sorted.
        self.correlations = {}
        
    def set_parameters(self, par_name, mean = None, width = None, range = None):
        if par_name not in self.parameters:
            self.add_parameter(par_name)
        values = list(self.parameters[par_name])
        if mean is not None:
           assert type(mean) == float
           values[0] = mean
        if width is not None:
           assert type(width) == float and width >= 0.0
           values[1] = width
        if range is not None:
           assert type(range[0]) == float and type(range[1]) == float
           values[2] = range[0], range[1]
        mean, width, range = values
        if mean < range[0] or mean > range[1]: raise RuntimeError, "set_parameters: mean is outside of range for parameter %s. This is not allowed." % par_name
        self.parameters[par_name] = tuple(values)
       
    def set_correlation_coefficient(self, par1, par2, rho):
        #if par1 not in self.parameters: raise RuntimeError, "set_correlation_coefficient: unknown parameter %s" % par1
        #if par2 not in self.parameters: raise RuntimeError, "set_correlation_coefficient: unknown parameter %s" % par2
        if rho <= -1.0 or rho >= 1.0: raise RuntimeError, "set_correlation_coefficient: correlation coefficient must be between -1 and 1"
        par1, par2 = sorted([par1, par2])
        if rho != 0.0: self.correlations[(par1, par2)] = rho
        elif (par1, par2) in self.correlations: del self.correlations[(par1, par2)]


    # *** "private" methods: ***
    # use dist0 as the standard distribution but use all correlations / gauss parameters
    # present in dist1 instead. Parameters present in dist1 but not in dist0 will be ignored.
    @staticmethod
    def merge(dist0, dist1):
        res = copy.deepcopy(dist0)
        for p in dist1.parameters:
            if p not in res.parameters: continue
            res.parameters[p] = dist1.parameters[p]
        for p1, p2 in dist1.correlations:
            if p1 not in res.parameters or p2 not in res.parameters: continue
            res.correlations[(p1, p2)] = dist1.correlations[(p1, p2)]
        return res
    
    
    def add_parameter(self, par_name):
        self.parameters[par_name] = 0.0, 1.0, (float("-inf"), float("inf"))
        
    def del_parameter(self, par_name):
        del self.parameters[par_name]
        for p1, p2 in self.correlations.keys():
            if par_name in (p1, p2): del self.correlations[(p1, p2)]
    
    
    # get theta model config dictionary suitable for a config file:
    def get_cfg(self):
        gausses1d = []
        flat_distribution = {'type': 'flat_distribution'}
        delta_distribution = {'type': 'delta_distribution'}
        gauss = {'type': 'gauss', 'parameters': [], 'ranges': []}
        
        parameters = sorted(self.parameters.keys())
        
        parameters_with_correlations = set()
        for p1, p2 in self.correlations:
            parameters_with_correlations.add(p1)
            parameters_with_correlations.add(p2)
        n_pcor = len(parameters_with_correlations)
        
        for p in parameters:
            if p in parameters_with_correlations: continue
            mean, width, range = self.parameters[p]
            if width==float("inf"):
                flat_distribution[p] = {'range': [range[0], range[1]], 'fix-sample-value': mean}
            elif width > 0 and range[0] != range[1]:
                gausses1d.append({'type': 'gauss1d', 'parameter': p, 'mean': mean, 'width': width, 'range': [range[0], range[1]]})
            else:
                delta_distribution[p] = mean
                
        covariance = numpy.ndarray((n_pcor, n_pcor))
        pcor = sorted(list(parameters_with_correlations))
        for i1 in xrange(len(pcor)):
            p1 = pcor[i1]
            p1_mean, p1_width, p1_range = self.parameters[p1]
            gauss['parameters'].append(p1)
            gauss['ranges'].append(p1_range)
            covariance[i1][i1] = p1_width**2
            for i2 in xrange(i1+1, len(pcor)):
                p2 = pcor[i2]
                p2_mean, p2_width, p2_range = self.parameters[p1]
                coeff = 0.0
                if (p1,p2) in self.correlations: coeff = self.correlations[(p1, p2)]
                covariance[i2][i1] = p1_width * p2_width * coeff
                covariance[i1][i2] = p1_width * p2_width * coeff
        gauss['covariance'] = covariance
        result = {'type': 'product_distribution', 'distributions':[]}
        for g in gausses1d: result['distributions'].append(g)
        if len(delta_distribution) > 1: result['distributions'].append(delta_distribution)
        if len(flat_distribution) > 1: result['distributions'].append(flat_distribution)
        if len(gauss['parameters']) > 0: result['distributions'].append(gauss)
        return result
    
    # returns a tuple (mean, width, range) for the given parameter    
    # if par_name is None, returns a list of parameters.
    def get_parameters(self, par_name = None):
        if par_name is None: return self.parameters.keys()
        return self.parameters[par_name]


def settingvalue_to_cfg(value, indent=0, errors = []):
    if type(value) == numpy.float64: value = float(value)
    if type(value) == str: return '"%s"' % value
    if type(value) == bool: return 'true' if value else 'false'
    if type(value) == int: return '%d' % value
    if type(value) == float:
        if value == float("inf"): return '"inf"'
        elif value == -float("inf"): return '"-inf"'
        return '%.5e' % value
    if type(value) == list or type(value) == tuple or type(value) == numpy.ndarray:
        return "(" + ",".join(map(lambda v: settingvalue_to_cfg(v, indent + 4), value)) + ')'
    if type(value) == dict:
        result = "{\n"
        # sort keys to make config files reproducible:
        for s in sorted(value.keys()):
            result += ' ' * (indent + 4) + s + " = " + settingvalue_to_cfg(value[s], indent + 4) + ";\n"
        return result + ' ' * indent + "}"
    errors.append("cannot convert type %s to cfg; writing '<type error>' to allow debugging" % type(value))
    return '<type error for type %s>' % type(value)

#note: the meaning of model is a bit different here w.r.t. the 'Model' class in theta;
# here, it is (as in theta) mainly a map: observable -> (list of coefficiencts, templates), but also
# * includes DATA histograms, if available
# * saves which process(es) are considered as signal
class Model:
    # "public" methods, to be used from analysis.py:
    def set_rate_impact(self, process_selector, uncertainty, relative_rate_impact):
        op = self.get_matching_processes(process_selector)
        if len(op)==0: raise RuntimeError, "no process matched '%s'" % process_selector
        if uncertainty not in self.rate_uncertainties:
            self.rate_uncertainties.add(uncertainty)
            self.rate_impact_types[uncertainty] = 'ln'
            self.distribution.add_parameter('delta_%s' % uncertainty)
        for observable, process in op:
            self.rate_impacts[(observable, process, uncertainty)] = relative_rate_impact

    def set_rate_impact_type_gauss(self, uncertainty):
        if uncertainty in self.shape_uncertainties: raise RuntimeError, "cannot make uncertainty '%s' gaussian because it is also used for histogram morphing" % uncertainty
        if uncertainty not in self.rate_uncertainties: raise RuntimeError, "rate uncertainty '%s' unknown" % uncertainty
        self.rate_impact_types[uncertainty] = 'g'
        self.sync_rate_impacts()

    def filter_histograms(self, patterns, keep_data_always = True):
        if type(patterns)==str: patterns = [patterns]
        if keep_data_always: patterns.append('*__DATA')
        for h in self.histos.keys():
            found_match = False
            for pat in patterns:
                if Model.simple_pattern_matches(pat, h):
                    found_match = True
                    break
            if not found_match:
                del self.histos[h]
        self.build_p_o_ob_su()

    def remove_histograms(self, patterns):
        if type(patterns)==str: patterns = [patterns]
        for h in self.histos.keys():
            found_match = False
            for pat in patterns:
                if Model.simple_pattern_matches(pat, h):
                    found_match = True
                    break
            if found_match:
                del self.histos[h]
        self.build_p_o_ob_su()
        
    def set_signal_processes(self, patterns):
        if type(patterns)==str: patterns = [patterns]
        self.signal_processes = set()
        for p in self.processes:
            found_match = False
            for pat in patterns:
                if Model.simple_pattern_matches(pat, p):
                    found_match = True
                    break
            if found_match: self.signal_processes.add(p)
        
    def scale_histograms(self, factor, patterns = ['*__*', '*__*__*__*'], scale_data = False):
        if type(patterns)==str: patterns = [patterns]
        for h in self.histos:
            if "__DATA" in h and not scale_data: continue
            found_match = False
            for pat in patterns:
                if Model.simple_pattern_matches(pat, h):
                    found_match = True
                    break
            if not found_match: continue
            for i in range(len(self.histos[h][2])):
                self.histos[h][2][i] = factor * self.histos[h][2][i]
                
                
    def convert_morph_uncertainties_to_rate(self):
        changes = False
        for o in self.observables:
            for p in self.processes:
                hn = self.histos[Model.hname(o, p)][2]
                for u in self.shape_uncertainties:
                    hp_name = Model.hname(o,p,u,'plus')
                    hm_name = Model.hname(o,p,u,'minus')
                    if hp_name not in self.histos or hm_name not in self.histos: continue
                    hp, hm = self.histos[hp_name][2], self.histos[hm_name][2]
                    factor_plus = sum(hp) / sum(hn)
                    factor_minus = sum(hm) / sum(hn)
                    if abs(2 - factor_minus - factor_plus) > 1e-6: continue
                    ok = True
                    for i in range(len(hn)):
                        if hn[i]==0:
                            ok = hp[i] == 0 and hm[i]==0
                            if not ok: break
                        reldiff1 = abs(hn[i] * factor_minus - hm[i])/hn[i]
                        reldiff2 = abs(hn[i] * factor_plus - hp[i])/hn[i]
                        ok = reldiff1 < 1e-6 and reldiff2 < 1e-6
                        if not ok: break
                    if ok:
                        changes = True
                        print "Note: Replacing shape changing uncertainty %s__%s__%s by a rate one" % (o,p,u)
                        self.set_rate_impact(o + "__" + p, u, factor_plus - 1)
                        del self.histos[hp_name]
                        del self.histos[hm_name]
        if changes: self.build_p_o_ob_su()
            
    # "private" methods, to be used from within this script, not from analysis.py:
    def __init__(self, root_filename):
        self.rf = rootfile(root_filename)
        # histos is a dictionary
        # string -> (float, float, list of floats)
        # histogram name -> histogram
        # and contains the histograms from the input root file.
        self.histos = {}
        # processes, observables, observable_binning and shape_uncertainties are
        # deduced from self.histos. Note that 'DATA' is not listed in processes, even if it exists.
        self.processes = set()
        self.signal_processes = set() #note: signal_processes is always a subset of processes.
        self.observables = set()
        # observable binning:
        # string -> (float, float, int)
        # observable -> (xmin, xmax, nbins)
        self.observable_binning = {}
        # shape_uncertainties are filled automatically via histograms, rate_uncertainties implicitely via set_rate_impact
        # uncertainties can be part of both.
        self.shape_uncertainties = set()
        
        # rate uncertainties; set via set_rate_imact* methods
        self.rate_uncertainties = set()
        # rate impacts is a dictionary
        # (string, string, string) -> float
        # (observable, process, uncertainty) -> rel. uncertainty
        self.rate_impacts = {}
        # rate_impact_types is a dictionary
        # string -> string
        # uncertainty -> type        where type is either "ln" or "g" for lognormal and gauss, resp.
        self.rate_impact_types = {}
        
        self.distribution = Distribution()
        histos = self.rf.get_histograms()
        for h in histos:
           l = h.split('__')
           observable, process, uncertainty, direction = [None]*4
           if len(l)==2: observable, process = l
           elif len(l)==4:
              observable, process, uncertainty, direction = l
           else:
              print "Warning: ignoring histogram %s which does not obey naming convention!" % h
              continue
           if direction not in (None, 'plus', 'minus'):
              print "Warning: ignoring histogram %s which does not obey naming convention!" % h
              continue
           self.histos[h] = self.rf.get_histogram(h)
           if process.startswith('signal'): self.signal_processes.add(process)
           if uncertainty is not None:
               pname = 'delta_%s' % uncertainty
               if not pname in self.distribution.parameters:
                   self.distribution.add_parameter(pname)
        self.build_p_o_ob_su()

                          
    def has_data(self):
        for o in self.observables:
            if Model.hname(o, 'DATA') not in self.histos: return False
        return True
    
    # build self.processes, self.signal_processes, self.observables, self.observable_binning, self.shape_uncertainties
    # based on the content of self.histograms.
    # Also cleanup self.rate_* variables and self.distribution from observables / processes no longer present
    def build_p_o_ob_su(self):
        # 1. build p, o, ob, su but use new sets instead of self directly no not make
        # modifications in case of errors:
        new_processes, new_observables, new_observable_binning, new_shape_uncertainties = set(), set(), {}, set()
        for h in self.histos:
           l = h.split('__')
           observable, process, uncertainty, direction = [None]*4
           if len(l)==2: observable, process = l
           elif len(l)==4:
              observable, process, uncertainty, direction = l
           new_observables.add(observable)
           new_processes.add(process)
           if uncertainty is not None:
               new_shape_uncertainties.add(uncertainty)
        # check histogram compatibility and that for each observable / process, there is a nominal histogram:
        for observable in new_observables:
            o_xmin, o_xmax, o_nbins = None, None, None
            for process in new_processes:
                hname_nominal = Model.hname(observable, process)
                if hname_nominal not in self.histos: continue
                for uncertainty in [None] + list(new_shape_uncertainties):
                    directions = ('plus', 'minus') if uncertainty is not None else [None]
                    for direction in directions:
                        hname = Model.hname(observable, process, uncertainty, direction)
                        if hname not in self.histos: continue
                        histo = self.histos[hname]
                        if o_xmin is None:
                            o_xmin, o_xmax, o_nbins, refname = histo[0], histo[1], len(histo[2]), Model.hname(observable, process, uncertainty, direction)
                            new_observable_binning[observable] = (o_xmin, o_xmax, o_nbins)
                        xmin, xmax, nbins = histo[0], histo[1], len(histo[2])
                        if xmin != o_xmin or xmax != o_xmax or nbins != o_nbins: raise RuntimeError, """for observable %s, not all histograms are compatible 
                          in range and binning: histo '%s' has (xmin, xmax, nbins) = (%f, %f, %d) while histo '%s' has (%f, %f, %d)""" % (observable, refname, o_xmin,
                          o_xmax, o_nbins, hname, xmin, xmax, nbins)
        self.processes = new_processes
        self.processes.discard('DATA')
        self.signal_processes.intersection_update(self.processes)
        self.observables = new_observables
        self.observable_binning = new_observable_binning
        self.shape_uncertainties = new_shape_uncertainties
        # 2.
        # 2.a. cleanup the three self.rate_* variables and self.distribution:
        for o,p,u in self.rate_impacts.keys():
            if o not in self.observables or p not in self.processes: del self.rate_impacts[(o,p,u)]
        new_rate_uncertainties = set()
        for o,p,u in self.rate_impacts: new_rate_uncertainties.add(u)
        self.rate_uncertainties = new_rate_uncertainties
        for u in self.rate_impact_types.keys():
            if u not in self.rate_uncertainties: del self.rate_impact_types[u]
        # 2.b. cleanup the distribution object:
        d_pars = self.distribution.get_parameters()
        for par in d_pars:
            assert par.startswith('delta_')
            u = par[len('delta_'):]
            if u not in self.rate_uncertainties and u not in self.shape_uncertainties: self.distribution.del_parameter(par)

    # synchronize the rate impact types with self.distribution by modifying self.distribution accordingly
    def sync_rate_impacts(self):
        for u in self.rate_uncertainties:
            if self.rate_impact_types[u] == 'ln': continue
            assert self.rate_impact_types[u] == 'g'
            rate_impacts = set()
            for observable, process, u2 in self.rate_impacts:
                if u != u2: continue
                rate_impacts.add(self.rate_impacts[(observable, process, u2)])
                if len(rate_impacts) > 1: raise RuntimeError, "uncertainty '%s' is gaussian but has different relative impact on the processes!" % u
            assert rate_impacts
            rate_impact = list(rate_impacts)[0]
            mean, width, range = 1.0, rate_impact, (0.0, float("inf"))
            self.distribution.set_parameters('delta_%s' % u, mean, width, range)
    
    # returns a list of two-tuples (observable,process), given the process selector.    
    def get_matching_processes(self, process_pattern):
        result = []
        for observable in self.observables:
            for process in self.processes:
                h = Model.hname(observable, process)
                if Model.simple_pattern_matches(process_pattern, h): result.append((observable, process))
        return result

    # returns whether the pattern matches the (whole) matchstring
    # pattern is a 'simple' pattern, as explained in README.txt. The only special
    # character is '*' which matches any observable/process/uncertainty name
    @staticmethod
    def simple_pattern_matches(pattern, matchstring):
        regex = pattern.replace("*", "(.+)")
        r = re.compile(regex)
        m = r.match(matchstring)
        if m is None or m.group() != matchstring:
            return False
        g = m.groups()
        for s in g:
            if '__' in s:
                return False
        return True
    
    @staticmethod
    def hname(observable, process, uncertainty = None, direction = None):
        if uncertainty is not None: return "%s__%s__%s__%s" % (observable, process, uncertainty, direction)
        else: return "%s__%s" % (observable, process)

class table:
   def __init__(self):
       self.columns = []
       self.pretty_colnames = []
       self.rows = []
       self.current_row = {}
       
   def add_column(self, colname, pretty_colname = None):
      if pretty_colname is None: pretty_colname = colname
      self.columns.append(colname)
      self.pretty_colnames.append(pretty_colname)
   
   def set_column(self, colname, value):
       self.current_row[colname] = value
   
   def add_row(self):
       row = []
       for c in self.columns: row.append(self.current_row[c])
       self.rows.append(row)
       self.current_row = {}

   def html(self):
       result = '<table cellpadding="2" border="1">\n<tr>'
       for c in self.pretty_colnames:
           result += '<th>%s</th>' % c
       result += '</tr>\n'
       for r in self.rows:
          result += '<tr><td>'
          result += '</td><td>'.join(r)
          result += '</td></tr>\n'
       result += '</table>\n'
       return result


def get_option(options, name, default = None, typecheck = True):
    if default is None: typecheck = False
    if name in options:
        value = options[name]
        if typecheck:
            if type(value) != type(default): raise RuntimeError, "option '%s' has wrong type: got %s, expected %s" % (name, type(value), type(default))
        return value
    return default
    
def set_option(options, name, value):
    options[name] = value


def model_summary_nuisance(dist, fname):
    f = open(fname, 'w')
    print >> f, "<h3>Marginal Priors</h3>"
    t = table()
    t.add_column('parameter')
    t.add_column('mean')
    t.add_column('width')
    t.add_column('range')
    for p in dist.parameters:
       mean, width, range = dist.parameters[p]
       t.set_column('parameter', p)
       t.set_column('mean', "%.5g" % mean)
       t.set_column('width', "%.5g" % width)
       t.set_column('range', "[%.5g, %.5g]" % range)
       t.add_row()
    print >> f, t.html()
    print >> f, "<h3>Correlations</h3>"
    t = table()
    t.add_column('parameter 1')
    t.add_column('parameter 2')
    t.add_column('correlation coefficient')
    n = 0
    for p1, p2 in dist.correlations:
        c = dist.correlations[(p1, p2)]
        assert c!=0
        t.set_column('parameter 1', p1)
        t.set_column('parameter 2', p2)
        t.set_column('correlation coefficient', "%.5g" % c)
        t.add_row()
        n += 1
    if n>0:
        print >>f, "<p>The non-zero correlation coefficients are:</p>", t.html()
    else:
        print >> f, "<p>All correlation coefficiencts of the nuisance parameters are zero.</p>"
    f.close()

# creates plots and model_plots.thtml
execfile("plot_results.py")
def model_plots(model, options):
    workdir = get_option(options, "workdir")
    assert workdir is not None
    plotdir = os.path.join(workdir, 'plots')
    observables = sorted(list(model.observables))
    processes = sorted(list(model.processes))
    #TODO: more / better colors
    background_colors = ['#edd400', '#f57900', '#c17d11', '#73d216', '#3465a4', '#75507b', '#d3d7cf', '#555753']
    signal_colors = ['#ef2929', '#cc0000', '#a40000']
    if not os.path.exists(plotdir): os.mkdir(plotdir)
    f = open(os.path.join(workdir, 'model_plots.thtml'), 'w')
    print >> f, "<h2>Stackplots</h2>"
    print >> f, "<p>Everything normalized to expectation, i.e., to the normalization in the template input file, possibly scaled via the python script file.</p>"
    print >> f, "<p>Color Code:</p><ul>"
    i_bkg_col = 0
    i_signal_col = 0
    for p in processes:
        if p in model.signal_processes:
            color = signal_colors[i_signal_col]
            i_signal_col = (i_signal_col + 1) % len(signal_colors)
        else:
            color = background_colors[i_bkg_col]
            i_bkg_col = (i_bkg_col + 1) % len(background_colors)
        print >> f, '<li><span style="background: %s;">&nbsp;&nbsp;&nbsp;</span> %s</li>' % (color, p)
    print >>f, '</ul>'
    units = get_option(options, 'units')
    if units is None: units = {}
    for o in observables:
        background_pds = []
        signal_pds = []
        i_bkg_col = 0
        i_signal_col = 0
        for p in processes:
            hname = Model.hname(o,p)
            if hname not in model.histos: continue
            xmin, xmax, data = model.histos[hname]
            binwidth = (xmax - xmin) / len(data)
            pd = plotdata()
            pd.x = [xmin + i*binwidth for i in range(len(data))]
            pd.y = data[:]
            if p in model.signal_processes:
                pd.color = signal_colors[i_signal_col]
                i_signal_col = (i_signal_col + 1) % len(signal_colors)
                signal_pds.append(pd)
            else:
                pd.fill_color = background_colors[i_bkg_col]
                pd.lw = 1
                pd.color = '#000000'
                i_bkg_col = (i_bkg_col + 1) % len(background_colors)
                background_pds.append(pd)
        data_pd = None
        hname = Model.hname(o,'DATA')
        if hname in model.histos:
            xmin, xmax, data = model.histos[hname]
            binwidth = (xmax - xmin) / len(data)
            data_pd = plotdata()
            data_pd.color = '#000000'
            data_pd.x = [xmin + i*binwidth for i in range(len(data))]
            data_pd.y = data[:]
            data_pd.yerrors = map(math.sqrt, data_pd.y)
            data_pd.circle = 'o'
        make_stack(background_pds)
        plots = background_pds + signal_pds
        if data_pd is not None: plots.append(data_pd)
        unit = ''
        if o in units: unit = ' ' + units[o]
        xlabel = ("%s $[%s]$" % (o, unit)) if unit != '' else o
        plot(plots, xlabel, '$N / %.4g%s$' % (binwidth, unit), os.path.join(plotdir, '%s_stack.png' % o), xmin=xmin, xmax=xmax)
        print >> f, "<p>Observable '%s':<br /><img src=\"plots/%s_stack.png\" /></p>" % (o, o)
        
    print >> f, "<h2>All 'nominal' Templates</h2>"
    print >> f, "<p>Everything normalized to expectation, i.e., to the normalization in the template input file, possibly scaled via the python script file.</p>"
    for o in observables:
        for p in processes:
            hname = Model.hname(o,p)
            if hname not in model.histos: continue
            xmin, xmax, data = model.histos[hname]
            binwidth = (xmax - xmin) / len(data)
            pd = plotdata()
            pd.x = [xmin + i*binwidth for i in range(len(data))]
            pd.y = data[:]
            pd.color = signal_colors[0]
            unit = ''
            if o in units: unit = ' ' + units[o]
            xlabel = ("%s $[%s]$" % (o, unit)) if unit != '' else o
            plot([pd], xlabel, '$N / %.4g%s$' % (binwidth, unit), os.path.join(plotdir, '%s_%s.png' % (o, p)), xmin=xmin, xmax=xmax)
            print >> f, '<p>Observable "%s", Process "%s":<br/><img src="plots/%s_%s.png"/></p>' % (o, p, o, p)
        # make also one plot with all signal processes, and normalization versus ordering:
        pd_norm = plotdata()
        pd_norm.x = []
        pd_norm.y = []
        pd_norm.as_histo = False
        pd_norm.color = '#000000'
        plots = []
        i_signal_col = 0
        for p in processes:
            if p not in model.signal_processes: continue
            hname = Model.hname(o,p)
            if hname not in model.histos: continue
            xmin, xmax, data = model.histos[hname]
            pd_norm.x.append(extract_number(p, options))
            pd_norm.y.append(sum(data))
            binwidth = (xmax - xmin) / len(data)
            pd = plotdata()
            pd.x = [xmin + i*binwidth for i in range(len(data))]
            pd.y = data[:]
            pd.color = signal_colors[i_signal_col]
            plots.append(pd)
            i_signal_col = (i_signal_col + 1) % len(signal_colors)
        unit = ''
        if o in units: unit = ' ' + units[o]
        xlabel = ("%s $[%s]$" % (o, unit)) if unit != '' else o
        plot(plots, xlabel, '$N / %.4g%s$' % (binwidth, unit), os.path.join(plotdir, '%s_signals.png' % o), xmin=xmin, xmax=xmax)
        plot([pd_norm], 'signal process', '$N$', os.path.join(plotdir, '%s_norm_vs_signals.png' % o))
        print >> f, '<p>Observable "%s", all signals: <br/><img src="plots/%s_signals.png"/></p>' % (o, o)
        print >> f, '<p>Observable "%s", signal normalization: <br/><img src="plots/%s_norm_vs_signals.png"/></p>' % (o, o)
        
    # shape comparison for morphed templates:
    color_nominal, color_plus, color_minus = '#333333', '#aa3333', '#3333aa'
    print >> f, "<h2>Shape Uncertainty Plots</h2>"
    print >> f, "<p>Color Code:</p><ul>"
    print >> f, "<li><span style=\"background: %s;\">&nbsp;&nbsp;&nbsp;</span> nominal</li><li><span style=\"background: %s;\">&nbsp;&nbsp;&nbsp;</span> plus</li><li><span style=\"background: %s;\">&nbsp;&nbsp;&nbsp;</span> minus</li>" % (color_nominal, color_plus, color_minus)
    print >> f, "</ul>"
    print >> f, "<p>Processes not appearing in the tables do not have any shape uncertainty for this observable.</p>"
    print >> f, "<p>Click on an image to enlarge. If you have javascript, the image will be displayed on this page and you can click through all shape uncertainties of that observable \
                   (instead of clicking, you can also use the left/right key on the keyboard).</p>"
    shape_uncertainties = sorted(list(model.shape_uncertainties))
    for o in observables:
        print >> f, '<h3>Observable \'%s\'</h3>' % o
        # save the triples (o,p,u) for which there is a plot:
        opus = []
        for p in processes:
            hname = Model.hname(o,p)
            if not hname in model.histos: continue
            for u in shape_uncertainties:
                hname_plus = Model.hname(o,p,u,'plus')
                hname_minus = Model.hname(o,p,u,'minus')
                if hname_plus not in model.histos or hname_minus not in model.histos: continue
                xmin, xmax, data_nominal = model.histos[hname]
                xmin, xmax, data_plus = model.histos[hname_plus]
                xmin, xmax, data_minus = model.histos[hname_minus]
                binwidth = (xmax - xmin) / len(data)
                pd = plotdata()
                pd.x = [xmin + i*binwidth for i in range(len(data_nominal))]
                pd.y = data_nominal
                pd.color = color_nominal
                pd_plus = plotdata()
                pd_plus.x = pd.x
                pd_plus.y = data_plus
                pd_plus.color = color_plus
                pd_minus = plotdata()
                pd_minus.x = pd.x
                pd_minus.y = data_minus
                pd_minus.color = color_minus
                name = '%s__%s__%s' % (o,p,u)
                unit = '' if o not in units else units[o]
                xlabel = ("%s $[%s]$" % (o, unit)) if unit != '' else o
                plot((pd, pd_plus, pd_minus), xlabel, '$N / %.4g%s$' % (binwidth, unit), os.path.join(plotdir, name + '.png'), xmin=xmin, xmax = xmax)
                opus.append((o,p,u))
        #make a table for this observable:
        t = table()
        t.add_column('process', 'process / uncertainty')
        us = sorted(list(set([u for o,p,u in opus])))
        ps = sorted(list(set([p for o,p,u in opus])))
        for u in us: t.add_column(u)
        for p in ps:
            t.set_column('process', p)
            for u in us:
                if (o,p,u) in opus:
                    t.set_column(u, '<a href="plots/%s__%s__%s.png" rel="lightbox[%s]"><img src="plots/%s__%s__%s.png" width="200"/></a>' % (o,p,u,o,o,p,u))
                else:
                    t.set_column(u, '---')
            t.add_row()
        print >>f, t.html()
        if len(opus)==0: print >> f, '<p>no shape uncertainties for this observable</p>'
    f.close()
                

def model_summary(report, model, nll_distribution, nll_distribution_minimizers, options = {}):
    create_figures = get_option(options, "create_figures", True)
    workdir = get_option(options, "workdir")
    assert workdir is not None
    observables = sorted(list(model.observables))
    processes = sorted(list(model.processes))
    #general info
    f = open(os.path.join(workdir, "model_summary_general.thtml"), 'w')
    print >>f, '<p>Observables (xmin, xmax, nbins):</p>\n<ul>'
    for o in observables:
        xmin, xmax, nbins = model.observable_binning[o]
        print >> f, '<li>%s (%.5g, %.5g, %d)</li>' % (o, xmin, xmax, nbins)
    print >> f, "</ul>"
    print >>f, '<p>Background processes:</p>\n<ul>'
    for p in processes:
        if p in model.signal_processes: continue
        print >> f, '<li>%s</li>' % p
    print >> f, "</ul>"
    print >> f, "<p>Signal processes:</p><ul>"
    for p in processes:
        if p in model.signal_processes: print >> f, '<li>%s</li>' % p
    print >> f, "</ul>"
    print >> f, '<p>Uncertainties:</p>\n<ul>'
    all_uncertainties = sorted(list(set(list(model.shape_uncertainties) + list(model.rate_uncertainties))))
    for u in all_uncertainties:
        if u in model.shape_uncertainties and u in model.rate_uncertainties: suffix = '(morph and rate)'
        elif u in model.shape_uncertainties: suffix = '(morph only)'
        elif u in model.rate_uncertainties: suffix = '(rate only)'
        print >> f, '<li>%s %s</li>' % (u, suffix)
    print >> f, "</ul>"
    f.close()
    
    #rate table
    rate_table = table()
    rate_table.add_column('process', 'process / observable')
    o_bkg_sum = {}
    for o in observables:
        rate_table.add_column(o)
        o_bkg_sum[o] = 0.0
    for p in processes:
        if p in model.signal_processes: continue
        rate_table.set_column('process', p)
        for o in observables:
           hname = Model.hname(o,p)
           if hname in model.histos:
               xmin, xmax, data = model.histos[hname]
           else:
               data = [0.]
           s = sum(data)
           o_bkg_sum[o] += s
           rate_table.set_column(o, '%.5g' % s)
        rate_table.add_row()
    rate_table.set_column('process', '<b>total background</b>')
    for o in observables: rate_table.set_column(o, '%.5g' % o_bkg_sum[o])
    rate_table.add_row()
    for p in processes:
        if p not in model.signal_processes: continue
        rate_table.set_column('process', p)
        for o in observables:
           hname = Model.hname(o,p)
           if hname in model.histos:
               xmin, xmax, data = model.histos[hname]
           else:
               data = [0.]
           s = sum(data)
           rate_table.set_column(o, '%.5g' % s)
        rate_table.add_row()
    #always show data row (even if there is no DATA ...):
    rate_table.set_column('process', '<b>DATA</b>')
    for o in observables:
        hname = Model.hname(o, 'DATA')
        if hname in model.histos: entry = '%.5g' % sum(model.histos[hname][2])
        else: entry = '---'
        rate_table.set_column(o, entry)
    rate_table.add_row()
    f = open(os.path.join(workdir, "model_summary_rates.thtml"), 'w')
    print >> f, "<p>Rates for all observables and processes as given by the 'nominal' templates:</p>"
    print >> f, rate_table.html()
    f.close()
    
    # rate impact of systematic uncertainties on the processes. One table per observable:
    f = open(os.path.join(workdir, "model_summary_rate_impacts.thtml"), 'w')
    print >> f, """<p>The table below summarize the impact of an uncertainty on the rate prediction of a process.</p>
    <p>For an uncertainty, (g) indicates this uncertainty is a 'rate only' truncated Gaussian uncertainty. Any other uncertainty can have either a shape-chaning
    effect or a lognormal rate changing effect on the process (or both).</p>
    <p>For the individual cells, (r) indicates the 'rate only' part of the uncertainty, (s) indicates the effect on the rate of an uncertainty treated via template morphing (i.e., the rate
    effect of the 'shape' uncertainty).<br/>
    The rate change in 'plus' direction of the uncertainty is written as superscript,
    the 'minus' direction as subscript.<br/>All numbers are in per cent.</p>"""
    for o in observables:
        print >> f, "<h2>Observable '%s'</h2>" % o
        rate_impact_table = table()
        rate_impact_table.add_column('process', 'process / uncertainty')
        all_uncertainties = sorted(list(set(list(model.shape_uncertainties) + list(model.rate_uncertainties))))
        for u in all_uncertainties:
            suffix = ''
            if u in model.rate_impact_types:
                t = model.rate_impact_types[u]
                if t == 'g': suffix = ' (g)'
                else: assert t == 'ln'
            rate_impact_table.add_column(u, u + suffix)
        for p in processes:
            hname_nominal = Model.hname(o, p)
            rate_impact_table.set_column('process', p)
            for u in all_uncertainties:
                splus, sminus = None, None
                hname_plus = Model.hname(o, p, u, 'plus')
                hname_minus = Model.hname(o, p, u, 'minus')
                if hname_plus in model.histos: splus = sum(model.histos[hname_plus][2]) / sum(model.histos[hname_nominal][2]) - 1
                if hname_minus in model.histos: sminus = sum(model.histos[hname_minus][2]) / sum(model.histos[hname_nominal][2]) - 1
                rplusminus = None
                if (o, p, u) in model.rate_impacts:
                    rplusminus = model.rate_impacts[(o,p,u)]
                cell = ''
                if splus is not None:
                    cell += '<sup>%+.4g</sup><sub>%+.4g</sub> (s) ' % (splus * 100, sminus * 100)
                if rplusminus is not None:
                    cell += '&#xb1;%.4g (r)' % (rplusminus * 100)
                if cell == '': cell = '---'
                rate_impact_table.set_column(u, cell)
            rate_impact_table.add_row()
        print >> f, rate_impact_table.html()
    f.close()
    
    # nuisance parameter priors
    model_summary_nuisance(model.distribution, os.path.join(workdir, "model_summary_nuisance.thtml"))
    model_summary_nuisance(nll_distribution, os.path.join(workdir, "model_summary_nuisance_nll.thtml"))
    model_summary_nuisance(nll_distribution_minimizers, os.path.join(workdir, "model_summary_nuisance_nll_minimizers.thtml"))

    # figures:
    create_plots = get_option(options, 'create_plots', True)
    if create_plots: model_plots(model, options)

    # build the index.html:
    report.new_section('General Model Info', file(os.path.join(workdir, 'model_summary_general.thtml')).read())
    report.new_section('Rate Summary', file(os.path.join(workdir, 'model_summary_rates.thtml')).read())
    report.new_section('Rate Impact of Systematic Uncertainties', file(os.path.join(workdir, 'model_summary_rate_impacts.thtml')).read())
    
    nuisance_priors =  """
    <p>The priors for the nuisance parameters are always Gaussian. As limit cases, Gaussians
    with infinite width or zero width are also valid which in effect are flat distributions
    or used to fix a parameter, respectively.</p>
    <h2>a. for toy data generation</h2>
    <div class="inner">%(model_summary_nuisance.thtml)s</div>
    <h2>b. for posterior definition of Bayesian methods</h2>
    <div class="inner">%(model_summary_nuisance_nll.thtml)s</div>
    <h2>c. as likelihood constraint terms for NLL-minimisation-based methods</h2>
    <div class="inner">%(model_summary_nuisance_nll_minimizers.thtml)s</div>
    </div>"""
    fnames = ['model_summary_nuisance.thtml', 'model_summary_nuisance_nll.thtml','model_summary_nuisance_nll_minimizers.thtml']
    d = {}
    for fname in fnames:
        f = open(os.path.join(workdir, fname), 'r')
        d[fname] = f.read()
        f.close()
    nuisance_priors = nuisance_priors % d
    report.new_section('Nuisance Parameter Priors', nuisance_priors)
    if create_plots:
        report.new_section('Basic Model Plots', file(os.path.join(workdir, 'model_plots.thtml')).read())

#histogram is a tuple (xmin, xmax, data). Returns a dictionary for cfg building
def get_histo_cfg(histogram):
    xmin, xmax, data = histogram
    return {'type': 'direct_data_histo', 'range': [xmin, xmax], 'nbins': len(data), 'data': data[:]}

#additional settings is a map
# string -> settingdict
#to write out as top-level settings in the config file
#cfg_filename is always relative to the workdir
def write_cfg(model, signal_process, options, additional_settings = {}, cfg_filename=None):
    assert signal_process=='' or signal_process in model.signal_processes
    workdir = get_option(options, 'workdir')
    config = ''
    config += "parameters = " + settingvalue_to_cfg(list(model.distribution.parameters.keys()) + ["beta_signal"]) + ";\n"
    obs = {}
    for o in model.observables:
        xmin, xmax, nbins = model.observable_binning[o]
        obs[o] = {'range': [xmin, xmax], 'nbins': nbins}
    config += "observables = " + settingvalue_to_cfg(obs) + ";\n"
    cfg_model = {}
    for o in model.observables:
        cfg_model[o] = {}
        for p in model.processes:
            if p in model.signal_processes and p!=signal_process: continue
            hnominal = Model.hname(o, p)
            if hnominal not in model.histos: continue
            uncertainties = []
            for u in model.shape_uncertainties:
                hname = Model.hname(o,p,u,'plus')
                if hname in model.histos: uncertainties.append(u)
            if len(uncertainties) > 0:
                histogram = {'type': 'cubiclinear_histomorph', 'parameters': map(lambda s: 'delta_%s' % s, uncertainties), 'nominal-histogram': get_histo_cfg(model.histos[hnominal])}
                for u in uncertainties:
                    histogram['delta_%s-plus-histogram' % u] = get_histo_cfg(model.histos[Model.hname(o,p,u,'plus')])
                    histogram['delta_%s-minus-histogram' % u] = get_histo_cfg(model.histos[Model.hname(o,p,u,'minus')])
            else:
                histogram = get_histo_cfg(model.histos[Model.hname(o,p)])
            cfg_model[o][p] = {'histogram': histogram}
            coefficient_function = {'type': 'multiply', 'factors': []}
            if p==signal_process: coefficient_function['factors'].append('beta_signal')
            for u in model.rate_uncertainties:
                if (o,p,u) not in model.rate_impacts: continue
                typ = model.rate_impact_types[u]
                if typ == 'g': coefficient_function['factors'].append("delta_%s" % u)
                else:
                   assert typ=='ln'
                   r = model.rate_impacts[(o,p,u)]
                   coefficient_function['factors'].append({'type': 'exp_function', 'lambda': r, 'parameter': 'delta_%s' % u})
            cfg_model[o][p]['coefficient-function'] = coefficient_function
    model_dist = {'type': 'product_distribution', 'distributions': ("@model-distribution-signal", model.distribution.get_cfg() )}
    cfg_model['parameter-distribution'] = model_dist
    config += "model = " + settingvalue_to_cfg(cfg_model) + ";\n"
    for s in additional_settings:
        config += s + " = " + settingvalue_to_cfg(additional_settings[s]) + ";\n"
    if cfg_filename is None: cfg_filename = 'model%s.cfg' % signal_process
    f = open(os.path.join(workdir, cfg_filename), 'w')
    print >>f, config
    f.close()

# collect information about the result of a statistical method applied to an ensemble of
# toys, such as upper limits on on ensemble without signal, etc.
class ensemble_summary:
    pass


def delta_distribution(**kwargs):
    result = {'type': 'delta_distribution'}
    for k in kwargs: result[k] = kwargs[k]
    return result
    
def product_distribution(*args):
    result = {'type': 'product_distribution'}
    result['distributions'] = args[:]
    return result
    
def sqlite_database(fname = ''):
    return {'type': 'sqlite_database', 'filename': fname}
    
def data_source(model):
    result = {'type': 'histo_source', 'name': 'source'}
    for o in model.observables:
        hname = Model.hname(o, 'DATA')
        result[o] = get_histo_cfg(model.histos[hname])
    return result

def info(s):
    if not suppress_info:
        print "[INFO] ", s
   
 
def md5sum(filename):
    m = hashlib.md5()
    f = open(filename, 'r')
    m.update(f.read())
    return m.hexdigest()

# cfg_names is a list of filenames (without ".cfg") which are expected to be located in the working directory
def run_theta(cfg_names, options):
    workdir = get_option(options, 'workdir')
    thetautils_dir = get_option(options, 'thetautils_dir')
    cache_dir = os.path.join(workdir, 'cache')
    if not os.path.exists(cache_dir): os.mkdir(cache_dir)
    theta = os.path.realpath(os.path.join(thetautils_dir, '..', 'bin', 'theta'))
    for name in cfg_names:
        cfgfile = name + '.cfg'
        dbfile = name + '.db'
        cfgfile_cache = os.path.join(cache_dir, cfgfile)
        dbfile_cache = os.path.join(cache_dir, dbfile)
        cfgfile_full = os.path.join(workdir, cfgfile)
        already_done = False
        if os.path.exists(cfgfile_cache) and os.path.exists(os.path.join(cache_dir, dbfile)):
            # compare the config files:
            already_done = md5sum(cfgfile_cache) == md5sum(cfgfile_full)
        if already_done:
            info("not running \"theta %s\": found up-to-date output in cache" % cfgfile)
            continue
        retval = os.system(theta + " --redirect-io=false " + cfgfile_full)
        if retval != 0:
            if os.path.exists(dbfile): os.unlink(dbfile)
            raise RuntimeError, "executing theta for cfg file '%s' failed with exit code %d" % (cfgfile, retval)
        # move to cache, also the config file ...
        shutil.move(dbfile, dbfile_cache)
        shutil.copy(cfgfile_full, cfgfile_cache)


class html_report:
    def __init__(self, output_filename, options):
        self.filename = output_filename
        self.f = open(output_filename, 'w')
        self.section_counter = 1
        self.section_active = False
        self.options = options
        self.f.write(
        """<?xml version="1.0" ?>
    <!DOCTYPE html PUBLIC "-//W3C//DTD XHTML 1.0 Transitional//EN" "http://www.w3.org/TR/xhtml1/DTD/xhtml1-transitional.dtd">
    <html xmlns="http://www.w3.org/1999/xhtml">
    <head><title>theta-auto model summary</title>
    <script type="text/javascript" src="js/prototype.js"></script>
    <script type="text/javascript" src="js/scriptaculous.js?load=effects,builder"></script>
    <script type="text/javascript" src="js/lightbox.js"></script>
    <link rel="stylesheet" href="css/lightbox.css" type="text/css" media="screen" />
    <style type="text/css">
    body {margin: 30px; font-family: Verdana, Arial, sans-serif; font-size: 12px; background:#eee;}
    h1, h2, h3 { padding: 0.2ex; padding-left:.8ex;}
    h1 {font-size: 150%%; background: #d44;}
    h2 {font-size: 110%%; background: #f88; padding-left:1.3ex;}
    h3 {font-size: 100%%; background: #aaa; padding-left:1.3ex;}
    .inner {padding-left: 3ex;}
    a img{ border: none;}
    p {margin:0 0 0 0; margin-top: 1.3ex;}
    ul {margin:0 0 0 0;}
    </style>
    </head><body>
    <p>Hint: click on top-level headers to toggle visibility of that section.</p>
    """)

    def new_section(self, heading, content = ''):
        if self.section_active: self.f.write('\n</div>\n')
        self.f.write('<h1 onclick=$(\'div%d\').toggle()>%d. %s</h1><div class="inner" id="div%d">\n' % (self.section_counter,
            self.section_counter, heading, self.section_counter))
        self.section_counter += 1
        self.f.write(content)
        self.section_active = True
        
    def add_p(self, text):
        self.f.write('<p>' + text + '</p>\n')
        
    def add_html(self, raw_html):
        self.f.write('<p>' + raw_html + '</p>\n')
        
    def close(self):
        if self.section_active: self.f.write('\n</div>\n')
        workdir = get_option(self.options, 'workdir')
        d = {}
        d['datetime'] = datetime.datetime.now().isoformat(' ')
        d['workdir'] = workdir
        self.f.write("<hr /><p>This page was generated at %(datetime)s for workdir '%(workdir)s'.</p>" % d)
        self.f.write('</body></html>')
        self.f.close()
        thetautils_dir = get_option(self.options, 'thetautils_dir')
        html_target_path = get_option(self.options, 'html_target_path', '')
        if html_target_path!='':
            shutil.copy(self.filename, html_target_path)
            shutil.rmtree(os.path.join(html_target_path, 'js'), ignore_errors = True)
            shutil.rmtree(os.path.join(html_target_path, 'css'), ignore_errors = True)
            shutil.rmtree(os.path.join(html_target_path, 'images'), ignore_errors = True)
            shutil.copytree(os.path.join(thetautils_dir, 'htmlstuff', 'js'), os.path.join(html_target_path, 'js'))
            shutil.copytree(os.path.join(thetautils_dir, 'htmlstuff', 'css'), os.path.join(html_target_path, 'css'))
            shutil.copytree(os.path.join(thetautils_dir, 'htmlstuff', 'images'), os.path.join(html_target_path, 'images'))
            if os.path.exists(os.path.join(workdir, 'plots')):
                shutil.rmtree(os.path.join(html_target_path, 'plots'), ignore_errors = True)
                shutil.copytree(os.path.join(workdir, 'plots'), os.path.join(html_target_path, 'plots'))
        
def get_mean_width(data):
    try: a = data[0] + 2
    except TypeError: data = [d[0] for d in data]
    mean = sum(data) / len(data)
    width = sum([(d - mean)**2 for d in data]) / (len(data) - 1)
    width = math.sqrt(width)
    return mean, width
    

def extract_number(s, options = {}):
    sp_to_x = get_option(options, 'sp_to_x')
    if sp_to_x is not None and s in sp_to_x: return float(sp_to_x[s])
    r = re.compile('(\d+)')
    m = r.search(s)
    return float(m.group(1))
    

# *** methods ***
# cfg naming convention: <command>-<signal name>-<input>[-<input seed>]
# where <input> is either "data" for data, "zero" for toys with beta_signal = 0.0,
#  "one"  for toys with beta_signal = 1.0
# <input seed> is the random seed for the data_source in case of input "zero" ot input "one".
def bayesian_limit(report, model, nll_distribution, options):
    signal_prior = get_option(options, 'signal_prior', {'type': 'flat_distribution', 'beta_signal': {'range': [1e-12, float("inf")],
       'fix-sample-value': 1.0}})
    main = {'n-events': 100, 'model': '@model', 'producers': ('@bayes_ul',), 'output_database': sqlite_database(), 'log-report': False}
    bayes_ul = {'type': 'mcmc_quantiles', 'name': 'bayes', 'parameter': 'beta_signal',
       'override-parameter-distribution': product_distribution("@signal_prior", "@nll_distribution"),
       'quantiles': [0.95], 'iterations': 20000 }
    cfg_options = {'plugin_files': ('$THETA_DIR/lib/core-plugins.so',)}
    toplevel_settings = {'nll_distribution': nll_distribution.get_cfg(), 'signal_prior': signal_prior,
                'model-distribution-signal': delta_distribution(beta_signal = 0.0), 'bayes_ul': bayes_ul, 'main': main,
                'options': cfg_options}
    cfg_names_to_run = []
    expected_result = get_option(options, 'expected_result', True)
    if model.has_data():
        main['data_source'] = data_source(model)
        main['n-events'] = 10
        for sp in model.signal_processes:
            name = 'bayes_limit-%s-data' % sp
            main['output_database']['filename'] = '%s.db' % name
            write_cfg(model, sp, options, toplevel_settings, cfg_filename = '%s.cfg' % name)
            cfg_names_to_run.append(name)
    if expected_result:
        seed = 1
        main['data_source'] = {'type': 'model_source', 'name': 'source', 'model': '@model', 'rnd_gen': {'seed': seed}}
        toplevel_settings['model-distribution-signal']['beta_signal'] = 0.0
        main['n-events'] = get_option(options, 'bayesian_limit_n-events_expected', 1000)
        for sp in model.signal_processes:
            name = 'bayes_limit-%s-zero-%d' % (sp, seed)
            main['output_database']['filename'] = '%s.db' % name
            write_cfg(model, sp, options, toplevel_settings, cfg_filename = '%s.cfg' % name)
            cfg_names_to_run.append(name)
    run_theta(cfg_names_to_run, options)
    workdir = get_option(options, 'workdir')
    cachedir = os.path.join(workdir, 'cache')
    plotsdir = os.path.join(workdir, 'plots')
    #expected results maps (process name) -> (median, band1, band2)
    # where band1 and band2 are tuples for the central 68 and 95%, resp.
    expected_results = {}
    observed_results = {}
    results = table()
    results.add_column('process')
    if expected_result: results.add_column('expected', 'expected (median / central 68%)')
    if model.has_data(): results.add_column('observed')
    for sp in sorted(list(model.signal_processes)):
        results.set_column('process', sp)
        if model.has_data():
            sqlfile = os.path.join(cachedir, 'bayes_limit-%s-data.db' % sp)
            data = sql(sqlfile, 'select bayes__quant09500 from products')
            mean, width = get_mean_width(data)
            results.set_column('observed', '%.3g +- %.3g' % (mean, width / math.sqrt(len(data))))
            #print "filling observed for ", sp
            observed_results[sp] = mean
        if expected_result:
            sqlfile = os.path.join(cachedir, 'bayes_limit-%s-zero-%d.db' % (sp, seed))
            data = sql(sqlfile, 'select bayes__quant09500 from products')
            data = sorted([d[0] for d in data])
            n = len(data)
            results.set_column('expected', '%.3g  (%.3g, %.3g)' % (data[n / 2], data[int(0.16 * n)], data[int(0.84 * n)]))
            #print "filling expected for ", sp
            expected_results[sp] = (data[n / 2], (data[int(0.16 * n)], data[int(0.84 * n)]), (data[int(0.025 * n)], data[int(0.975 * n)]))
        results.add_row()
    report.new_section('Bayesian Limits')
    if expected_result: report.add_p('The expected result is the median and central 68% of toy outcomes.')
    if model.has_data(): report.add_p('The uncertainty quoted for the "observed" result is due to limited chain length in MCMC integration only.')
    report.add_html(results.html())
    if not expected_result: return
    # map process names and x values:
    sp_to_x = {}
    x_to_sp = {}
    for sp in model.signal_processes:
        x = extract_number(sp, options)
        sp_to_x[sp] = x
        x_to_sp[x] = sp
    pd = plotdata()
    pd.color = '#000000'
    pd.as_function = True
    pd.x = sorted(list(x_to_sp.keys()))
    pd.y = []
    pd.bands = [([], [], '#00ff00'), ([], [], '#00aa00')]
    #print "expected results: ", expected_results
    #print "observed results: ", observed_results
    for x in pd.x:
        sp = x_to_sp[x]
        median, band1, band2 = expected_results[sp]
        if model.has_data(): pd.y.append(observed_results[sp])
        else: pd.y.append(median)
        pd.bands[1][0].append(band1[0])
        pd.bands[1][1].append(band1[1])
        pd.bands[0][0].append(band2[0])
        pd.bands[0][1].append(band2[1])
    plot((pd,), 'signal process', '95% C.L. upper limit', os.path.join(plotsdir, 'bayesian_limit.png'))
    report.add_html('<p><img src="plots/bayesian_limit.png" /></p>')

    

# posteriors of all parameters
def posteriors(report, model, nll_distribution, options):
    #signal_prior = get_option(options, 'signal_prior', {'type': 'flat_distribution', 'beta_signal': {'range': [1e-12, float("inf")],
    #   'fix-sample-value': 1.0}})
    main = {'n-events': 3, 'model': '@model', 'producers': ('@posteriors',), 'output_database': sqlite_database(), 'log-report': False}
    posteriors = {'type': 'mcmc_posterior_histo', 'name': 'p', 'parameters': [],
       'override-parameter-distribution': "@nll_distribution",
       'smooth': True, 'iterations': 20000 }
    for p in model.distribution.parameters:
        posteriors['parameters'].append(p)
        # TODO: choose better ranges, especially for non-delta parameters
        r = get_option(options, '%s_range', [-3.0, 3.0] if p.startswith('delta') else [0.0, 3.0])
        posteriors['histo_%s' % p] = {'range': r, 'nbins': 100}
    
    cfg_options = {'plugin_files': ('$THETA_DIR/lib/core-plugins.so',)}
    toplevel_settings = {'nll_distribution': nll_distribution.get_cfg(),
                'posteriors': posteriors, 'main': main, 'model-distribution-signal': dict(type = 'delta_distribution'),
                'options': cfg_options}
    cfg_names_to_run = []
    if model.has_data():
        main['data_source'] = data_source(model)
        name = 'posteriors-data'
        main['output_database']['filename'] = '%s.db' % name
        write_cfg(model, '', options, toplevel_settings, cfg_filename = '%s.cfg' % name)
        cfg_names_to_run.append(name)
    run_theta(cfg_names_to_run, options)
    workdir = get_option(options, 'workdir')
    cachedir = os.path.join(workdir, 'cache')
    
    results = table()
    results.add_column('parameter')
    results.add_column('most probable value')
    results.add_column('1sigma most probable interval')
    sqlfile = os.path.join(cachedir, 'posteriors-data.db')
    for p in sorted(list(model.distribution.parameters.keys())):
        results.set_column('parameter', p)
        data = sql(sqlfile, 'select p__posterior_%s from products' % p)
        histos = [plotdata_from_histoColumn(d[0]) for d in data]
        maxima = [findmax(pd) for pd in histos]
        mean, width = get_mean_width(maxima)
        results.set_column('most probable value', '%.3g +- %.3g' % (mean, width / math.sqrt(len(maxima))))
        interval = central_1sigma(histos[0])
        results.set_column('1sigma most probable interval', "[%.3g,%.3g]" % (interval[0], interval[1]))
        results.add_row()
    report.new_section('Posteriors')
    report.add_p('Most probable values / intervals without any signal.')
    report.add_html(results.html())
    #TODO: make plots with template-functions evaluated at the mpv (!)
    

def main():
    scriptname = 'analysis.py'
    rootfilename = 'templates.root'
    command = 'model_summary'
    #TODO: better command-line parsing ...
    for arg in sys.argv[1:]:
        if '.py' in arg: scriptname = arg
        elif '.root' in arg: rootfilename = arg
        else: command = arg
    model = Model(rootfilename)
    global suppress_info
    suppress_info = False
    variables = {}
    options = {}
    variables['model'] = model
    variables['nll_distribution'] = Distribution()
    variables['set_option'] = lambda n, v: set_option(options, n, v)
    if os.path.exists(scriptname):
        info("executing script %s" % scriptname)
        try:
            execfile(scriptname, variables)
        except Exception as e:
            print "error while trying to execute analysis script %s:" % scriptname
            traceback.print_exc()
            sys.exit(1)
    nll_distribution = Distribution.merge(model.distribution, variables['nll_distribution'])
    fix_all_pars = Distribution()
    for p in nll_distribution.get_parameters():
        mean, width, range = nll_distribution.get_parameters(p)
        fix_all_pars.add_parameter(p)
        fix_all_pars.set_parameters(p, mean = mean, width = 0.0, range = [mean, mean])
    nll_distribution_minimizers = Distribution.merge(fix_all_pars, variables['nll_distribution'])

    workdir = get_option(options, 'workdir', os.path.join(os.getcwd(), scriptname[:-3]))
    workdir = os.path.realpath(workdir)
    set_option(options, 'workdir', workdir)
    if not os.path.exists(workdir): os.mkdir(workdir)
    
    #TODO: correct ...
    thetautils_dir = os.getcwd()
    options['thetautils_dir'] = thetautils_dir
    
    report = html_report(os.path.join(workdir, 'index.html'), options)
    command = get_option(options, 'command', command, typecheck = False)
    if type(command)==str: commands = [command]
    else: commands = command
    for command in commands:
        info("executing command " + command)
        if command == 'model_summary': model_summary(report, model, nll_distribution, nll_distribution_minimizers, options)
        elif command == 'model_cfg':
            for sp in [''] + list(model.signal_processes):
                write_cfg(model, sp, options)
        elif command == 'bayesian_limit':
            bayesian_limit(report, model, nll_distribution, options)
        elif command == 'posteriors':
            posteriors(report, model, nll_distribution, options)
        else: raise RuntimeError, "unknown command %s" % command
    report.close()

main()

