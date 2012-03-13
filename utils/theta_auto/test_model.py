# -*- coding: utf-8 -*-
# this file defines some simple models useful for testing theta-auto

from Model import *


# returns a model with a counting experiment in one bin with the given backgruond with a log-normal uncertainty.
# b_uncertainty is the absolute uncertainty on b.
#
# If s2 is not None, will return a model with two signal processes, "s" and "s2"
def simple_counting(s, n_obs, b=0.0, b_uncertainty=0.0, s2 = None):
    model = Model()
    model.set_data_histogram('obs', (0.0, 1.0, [float(n_obs)]))
    hf_s = HistogramFunction()
    hf_s.set_nominal_histo((0.0, 1.0, [float(s)]))
    model.set_histogram_function('obs', 's', hf_s)
    if s2 is not None:
        hf_s2 = HistogramFunction()
        hf_s2.set_nominal_histo((0.0, 1.0, [float(s2)]))
        model.set_histogram_function('obs', 's2', hf_s2)
    model.set_signal_processes('s*')
    if b > 0:
        hf_b = HistogramFunction()
        hf_b.set_nominal_histo((0.0, 1.0, [float(b)]))
        model.set_histogram_function('obs', 'b', hf_b)
        if b_uncertainty > 0: model.add_lognormal_uncertainty('bunc', 1.0 * b_uncertainty / b, 'b')
    return model


# returns a model in one bin with the given background. b_plus and b_minus are the background yields at +1sigma and -1sigma
def simple_counting_shape(s, n_obs, b=0.0, b_plus=0.0, b_minus=0.0):
    model = Model()
    model.set_data_histogram('obs', (0.0, 1.0, [float(n_obs)]))
    hf_s = HistogramFunction()
    hf_s.set_nominal_histo((0.0, 1.0, [float(s)]))
    model.set_histogram_function('obs', 's', hf_s)
    model.set_signal_processes('s*')
    if b > 0:
        hf_b = HistogramFunction()
        hf_b.set_nominal_histo((0.0, 1.0, [float(b)]))
        plus_histo = (0.0, 1.0, [float(b_plus)])
        minus_histo = (0.0, 1.0, [float(b_minus)])
        hf_b.set_syst_histos('bunc', plus_histo, minus_histo)
        model.distribution.set_distribution('bunc', 'gauss', mean = 0.0, width = 1.0, range = [-float("inf"), float("inf")])
        model.set_histogram_function('obs', 'b', hf_b)
    return model


# simple counting model with s signal events, no background, but an BB uncertainty on the signal of s_uncertainty (an absolute uncertainty)
def simple_counting_bb(s, s_uncertainty, n_obs):
    model = Model()
    model.set_data_histogram('obs', (0.0, 1.0, [float(n_obs)]))
    hf_s = HistogramFunction()
    hf_s.set_nominal_histo((0.0, 1.0, [float(s)]))
    hf_s.set_nominal_uncertainty_histo((0.0, 1.0, [float(s_uncertainty)]))
    model.set_histogram_function('obs', 's', hf_s)
    model.set_signal_processes('s*')
    model.bb_uncertainties = True
    return model
    
# in this case, s, s_uncertainty and n_obs should all be lists of the same length of floating point values.
def template_counting_bb(s, s_uncertainty, n_obs):
    assert len(s) == len(s_uncertainty) and len(s) == len(n_obs)
    model = Model()
    model.set_data_histogram('obs', (0.0, 1.0, n_obs))
    hf_s = HistogramFunction()
    hf_s.set_nominal_histo((0.0, 1.0, s))
    hf_s.set_nominal_uncertainty_histo((0.0, 1.0, s_uncertainty))
    model.set_histogram_function('obs', 's', hf_s)
    model.set_signal_processes('s*')
    model.bb_uncertainties = True
    return model


# a gaussian signal (mean 50, width 20) over flat background on a range 0--100, with 100 bins no data.
# b_uncertainty is the (absolute!) uncertainty on the background yield, which will be handeled with a log-normal.
def gaussoverflat(s, b, b_uncertainty = 0.0):
    model = Model()    
    # something proportional to a Gauss:
    gauss = lambda x, mean, sigma: math.exp(-(x - mean)**2 / (2.0*sigma))
    hf_s = HistogramFunction()
    data = [gauss(x+0.5, 50, 20) for x in range(100)]
    # normalize data to s:
    n_data = sum(data)
    data = [x*s/n_data for x in data]
    hf_s.set_nominal_histo((0.0, 100.0, data))
    model.set_histogram_function('obs', 's', hf_s)
    
    hf_b = HistogramFunction()
    data = [b * 0.01] * 100
    hf_b.set_nominal_histo((0.0, 100.0, data))
    model.set_histogram_function('obs', 'b', hf_b)
    if b_uncertainty > 0: model.add_lognormal_uncertainty('bunc', 1.0 * b_uncertainty / b, 'b')
    
    model.set_signal_processes('s*')
    return model
    
