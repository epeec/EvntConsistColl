#ifndef COMMON_H
#define COMMON_H

template <typename T> static void swap(T *a, T *b) {
  T tmp = *a;
  *a = *b;
  *b = tmp;
}

template <typename T> void sort_median(T *begin, T *end) {
  T *ptr;
  T *split;
  if (end - begin <= 1)
    return;
  ptr = begin;
  split = begin + 1;
  while (++ptr != end) {
    if (*ptr < *begin) {
      swap(ptr, split);
      ++split;
    }
  }
  swap(begin, split - 1);
  sort_median(begin, split - 1);
  sort_median(split, end);
}

template <typename T> void fill_array(const int n, T a[]) {
    gaspi_rank_t iProc;
    SUCCESS_OR_DIE( gaspi_proc_rank(&iProc) );

    for (int i=0; i < n; i++) {
        a[i] = i + iProc + 1;
    }
}

template <typename T> void fill_array_zeros(const int n, T a[]) {
    for (int i=0; i < n; i++) {
        a[i] = 0;
    }
}

template <typename T> double calculateMean(const int n, const T* a) {
    double sum = 0.0, mean;

    // compute mean
    int i;
    for(i = 0; i < n; ++i) {
        sum += a[i];
    }
    mean = sum / n;

    return mean;
}

template <typename T> double calculateConfidenceLevel(const int n, const T* a, const double mean) {
    double standardDeviation = 0.0, confidenceLevel;

    int i;
    // compute standard deviation
    for(i = 0; i < n; ++i)
        standardDeviation += pow(a[i] - mean, 2);
    standardDeviation = sqrt(standardDeviation / n);

    // compute confidence level of 95%
    confidenceLevel = 1.96 * (standardDeviation / sqrt((double) n));
    
    return confidenceLevel;
}
#endif
