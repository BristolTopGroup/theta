#include "interface/mcmc-result.hpp"
#include "interface/exception.hpp"
#include <cmath>
#include <sstream>

using namespace theta;

/*Result implementation: */
Result::Result(size_t n) :
    npar(n), count(0), count_different_points(0), sum(n), sumsquares(n, n) {
}

void Result::reset() {
    for (size_t i = 0; i < npar; i++) {
        sum[i] = 0;
        for (size_t j = 0; j < npar; j++) {
            sumsquares(i, j) = 0;
        }
    }
    count = count_different_points = 0;
}

Result::~Result() {
}

void Result::end() {
}

void Result::fill(const double * p, double nll, size_t weight) throw () {
    for (size_t i = 0; i < npar; i++) {
        double tmp;
        sum[i] += tmp = weight * p[i];
        for (size_t j = i; j < npar; j++) {
            sumsquares(i, j) += tmp * p[j];
        }
    }
    count += weight;
    count_different_points++;
}

void Result::combineResult(const Result & res) {
    if (npar != res.npar)
        throw InvalidArgumentException("Result::combineResult: dimensions mismatch");
    for (size_t i = 0; i < npar; i++) {
        sum[i] += res.sum[i];
        for (size_t j = 0; j < npar; j++) {
            sumsquares(i, j) += res.sumsquares(i, j);
        }
    }
    count += res.count;
    count_different_points += res.count_different_points;
}
//these usually do not need to be overrided:

size_t Result::getnpar() const {
    return npar;
}

size_t Result::getCount() const {
    return count;
}

size_t Result::getCountDifferent() const {
    return count_different_points;
}

std::vector<double> Result::getMeans() const {
    std::vector<double> result(npar);
    for (size_t i = 0; i < npar; i++) {
        result[i] = sum[i] / count;
    }
    return result;
}

std::vector<double> Result::getSigmas() const {
    std::vector<double> result(npar);
    for (size_t i = 0; i < npar; i++) {
        result[i] = sqrt((sumsquares(i, i) - sum[i] * sum[i] / count) / (count - 1));
    }
    return result;
}

Matrix Result::getCov() const {
    Matrix result(npar, npar);
    for (size_t i = 0; i < npar; i++) {
        for (size_t j = i; j < npar; j++) {
            result(j, i) = result(i, j) = (sumsquares(i, j) - sum[i] * sum[j] / count) / (count - 1);
        }
    }
    return result;
}

/* FullResult Mem implementation: */
inline void FullResultMem::ensureCapacity(size_t cap) {
    if (nallocvec < cap) {
        if (nallocvec == 0) {
            nallocvec = cap;
            results = (double*) malloc(nallocvec * npar * sizeof(double));
            counts = (size_t*) malloc(nallocvec * sizeof(size_t));
        } else {
            nallocvec = std::max(nallocvec * 2, cap);
            results = (double*) realloc(results, nallocvec * npar * sizeof(double));
            counts = (size_t*) realloc(counts, nallocvec * sizeof(size_t));
        }
        if (results == 0 || counts == 0) {
            std::stringstream s;
            s << "FullResultMeme::ensureCapacity: could not reserve requested space (for" << cap << " entries)";
            throw Exception(s.str());
        }
    }
}

FullResultMem::FullResultMem(size_t n, size_t cap/*=1024*/) :
    Result(n) {
    nallocvec = 0;
    ensureCapacity(cap);
}

FullResultMem::~FullResultMem() {
    free( counts);
    free( results);
}

void FullResultMem::fill(const double * p, double nll, size_t weight) {
    ensureCapacity(count_different_points + 1);
    counts[count_different_points] = weight;
    double *rres = results + count_different_points * npar;
    for (size_t i = 0; i < npar; i++) {
        double tmp;
        sum[i] += tmp = weight * (rres[i] = p[i]);
        for (size_t j = i; j < npar; j++) {
            sumsquares(i, j) += tmp * p[j];
        }
    }
    count += weight;
    count_different_points++;
}

const double* FullResultMem::getResult(size_t i) const {
    return results + i * npar;
}

size_t FullResultMem::getCountRes(size_t i) const {
    return counts[i];
}

HistoResult::HistoResult(const std::vector<std::pair<double, double> > & ranges, const std::vector<size_t> & nbins) :
    Result(ranges.size()), histos(ranges.size()) {
    if (ranges.size() != nbins.size())
        throw InvalidArgumentException("HistoResult::HistoResult: ranges and nbins sizes mismatch!");
    for (size_t i = 0; i < npar; i++) {
        if (nbins[i] != 0)
            histos[i].reset(nbins[i], ranges[i].first, ranges[i].second);
    }
}

void HistoResult::fill(const double * p, double nll, size_t weight) {
    for (size_t i = 0; i < npar; i++) {
        double tmp;
        histos[i].fill(p[i], weight);
        sum[i] += tmp = weight * p[i];
        for (size_t j = i; j < npar; j++) {
            sumsquares(i, j) += tmp * p[j];
        }
    }
    count += weight;
    count_different_points++;
}

HistoResult::~HistoResult() {
}

void HistoResult::reset() {
    Result::reset();
    for (size_t i = 0; i < npar; i++) {
        histos[i].reset();
    }
}

const Histogram & HistoResult::getHistogram(size_t i) const {
    return histos[i];
}

