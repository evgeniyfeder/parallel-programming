#include <cilk/cilk.h>

#include <algorithm>
#include <chrono>
#include <iostream>
#include <random>
#include <vector>

namespace seq
{
   int partition(std::vector<int> & a, int l, int r)
   {
      int pivot = a[r];
      int i = l - 1;
      for (int j = l; j < r; j++)
      {
         if (a[j] < pivot)
         {
            i++;
            std::swap(a[i], a[j]);
         }
      }
      std::swap(a[i + 1], a[r]);
      return i + 1;
   }

   void quicksort(std::vector<int> & a, int l, int r)
   {
      if (l < r)
      {
         int p = partition(a, l, r);
         quicksort(a, l, p - 1);
         quicksort(a, p + 1, r);
      }
   }
}

namespace par
{
   void quicksort(std::vector<int> & a, int l, int r)
   {
      if (r - l < 1000)
      {
         seq::quicksort(a, l, r);
         return;
      }
      if (l < r)
      {
         int p = seq::partition(a, l, r);
         cilk_spawn quicksort(a, l, p - 1);
         quicksort(a, p + 1, r);
         cilk_sync;
      }
   }
}

int main()
{
   const int n = 100000000, m = 5;

   std::mt19937 rng(std::chrono::steady_clock::now().time_since_epoch().count());
   std::uniform_int_distribution<int> dist(0, 1000000);

   std::pair<long, long> sum_time = {0,0};

   for (int i = 0; i < m; i++)
   {
      std::cout << "Test  " << i << std::endl;
      std::vector<int> arr_seq, arr_par, arr_std;

      arr_seq.reserve(n);
      arr_par.reserve(n);
      arr_std.reserve(n);
      for (int j = 0; j < n; j++)
      {
         int x = dist(rng);

         arr_seq.push_back(x);
         arr_par.push_back(x);
         arr_std.push_back(x);
      }

      std::chrono::steady_clock::time_point begin = std::chrono::steady_clock::now();
      par::quicksort(arr_par, 0, arr_par.size() - 1);
      std::chrono::steady_clock::time_point end = std::chrono::steady_clock::now();
      auto time = std::chrono::duration_cast<std::chrono::microseconds>(end - begin).count();
      std::cout << "Time par = " << time << " µs" << std::endl;
      sum_time.first += time;

      begin = std::chrono::steady_clock::now();
      seq::quicksort(arr_seq, 0, arr_seq.size() - 1);
      end = std::chrono::steady_clock::now();
      time = std::chrono::duration_cast<std::chrono::microseconds>(end - begin).count();
      std::cout << "Time seq = " << time << " µs" << std::endl;
      sum_time.second += time;

      std::sort(arr_std.begin(), arr_std.end());
      for (int j = 0; j < n; j++)
         if (arr_seq[j] != arr_std[j] || arr_par[j] != arr_std[j])
         {
            std::cout << ":(" << std::endl;
            return 0;
         }
   }
   std::cout << "===================" << std::endl;
   std::cout << "Average time par = " << sum_time.first / m << " µs" << std::endl;
   std::cout << "Average time seq = " << sum_time.second / m << " µs" << std::endl;
   std::cout << "Ratio seq/par = " << (double)sum_time.second / sum_time.first << std::endl;

   return 0;
}