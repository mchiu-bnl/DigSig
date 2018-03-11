#ifndef __RUNNINGSTATS_H__
#define __RUNNINGSTATS_H__
 
/**
 * Class to calculate running average and RMS
 * Taken from https://www.johndcook.com/blog/skewness_kurtosis/
 */
class RunningStats
{
public:
  RunningStats();
  void Clear();
  void Push(double x);
  long long NumDataValues() const;
  double Mean() const;
  double Variance() const;
  double StandardDeviation() const;
  double RMS() const;
  double Skewness() const;
  double Kurtosis() const;

  friend RunningStats operator+(const RunningStats a, const RunningStats b);
  RunningStats& operator+=(const RunningStats &rhs);

private:
  long long n;
  double M1, M2, M3, M4;
};

#endif // __RUNNINGSTATS_H__
