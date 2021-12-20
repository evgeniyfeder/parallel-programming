#include <datapar.hpp>

#include <cassert>
#include <iostream>
#include <limits>
#include <queue>
#include <vector>

using graph_t = std::vector<std::vector<std::size_t>>;


namespace seq
{
   std::int32_t bfs(graph_t const & graph, std::size_t from, std::size_t to)
   {
      std::queue<std::size_t> q;

      std::vector<std::int32_t> d(graph.size(), -1);

      q.push(from);
      d[from] = 0;
      while (!q.empty())
      {
         std::size_t cur = q.front();
         q.pop();

         for (auto const & next : graph[cur])
         {
            if (d[next] == -1)
            {
               d[next] = d[cur] + 1;
               q.push(next);
            }
         }
      }
      return d[to];
   }
}

namespace par
{
   using namespace pasl::pctl;


   int bfs(graph_t & graph, int from, int to)
   {
      parray<int> d(graph.size(), -1);

      __atomic_store_n(&d[from], 0, __ATOMIC_SEQ_CST);
      parray<int> frontier = {from};

      while (frontier.size() != 0)
      {
         long n = frontier.size();
         parray<int> degree(n, [&graph, &frontier](int i) {return graph[frontier[i]].size();});

         // count pref sums of degree
         auto combine = [] (int x, int y) {
            return x + y;
         };
         parray<int> pref_degree = scan(degree.begin(), degree.end(), 0, combine, forward_exclusive_scan);

         long next_frontier_size = pref_degree[pref_degree.size() - 1] + degree[degree.size() - 1];
         parray<int> next_frontier(next_frontier_size, -1);

         auto complexity_rng = [&] (long lo, long hi) {
            long hi_value = next_frontier_size;

            if (hi < pref_degree.size())
               hi_value = pref_degree[hi];

            return hi_value - pref_degree[lo];
         };

         auto complexity = [&] (long i) {
            return graph[frontier[i]].size();
         };

         parallel_for((long)0, n, [&frontier, &graph, &next_frontier, &pref_degree, &d] (long i)
         {
            int v = frontier[i];
            auto & out_vs = graph[v];
            for (int j = 0; j < out_vs.size(); ++j)
            //parallel_for((long)0, (long)out_vs.size(), [&] (long j)
            {
               int u = (int)out_vs[j];

               int expected = -1;
               int want = __atomic_load_n(&d[v], __ATOMIC_SEQ_CST) + 1;
               if (__atomic_compare_exchange_n(&d[u], &expected, want, false, __ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST))
               {
                  next_frontier[pref_degree[i] + j] = u;
               }
            }//);
         });
         frontier = filter(next_frontier.begin(), next_frontier.end(), [](int x) {return x != -1;});
      }
      return d[to];
   }
}


void add_edge(graph_t & graph, int x, int y, int z, int dx, int dy, int dz, std::size_t cube_size)
{
   if (x + dx < 0 || x + dx >= cube_size || y + dy < 0 || y + dy >= cube_size || z + dz < 0 || z + dz >= cube_size)
      return;

   int a = x + y * cube_size + z * cube_size * cube_size;
   int b = x + dx + (y + dy) * cube_size + (z + dz) * cube_size * cube_size;
   graph[a].push_back(b);
   graph[b].push_back(a);
}

graph_t make_cubic_graph(std::size_t cube_size)
{
   std::size_t n = cube_size * cube_size * cube_size;

   graph_t graph(cube_size * cube_size * cube_size);

   for (int z = 0; z < cube_size; ++z)
      for (int y = 0; y < cube_size; ++y)
         for (int x = 0; x < cube_size; x++)
         {
            add_edge(graph, x, y, z, 1, 0, 0, cube_size);
            add_edge(graph, x, y, z, 0, 1, 0, cube_size);
            add_edge(graph, x, y, z, 0, 0, 1, cube_size);
         }
   return graph;
}

int main()
{
   const int n = 300, m = 5;

   std::pair<long, long> sum_time = {0,0};
   auto graph = make_cubic_graph(n);

   for (int i = 0; i < m; i++)
   {
      std::cout << "Test  " << i << std::endl;

      std::chrono::steady_clock::time_point begin = std::chrono::steady_clock::now();

      std::int32_t d_par = par::bfs(graph, 0, graph.size() - 1);

      std::chrono::steady_clock::time_point end = std::chrono::steady_clock::now();
      auto time = std::chrono::duration_cast<std::chrono::microseconds>(end - begin).count();
      std::cout << "Time par = " << time << " µs" << std::endl;
      sum_time.first += time;

      begin = std::chrono::steady_clock::now();
      std::int32_t d_seq = seq::bfs(graph, 0, graph.size() - 1);

      end = std::chrono::steady_clock::now();
      time = std::chrono::duration_cast<std::chrono::microseconds>(end - begin).count();
      std::cout << "Time seq = " << time << " µs" << std::endl;
      sum_time.second += time;

      assert(d_seq == 3 * (n - 1) && d_par == d_seq);
   }
   std::cout << "===================" << std::endl;
   std::cout << "Average time par = " << sum_time.first / m << " µs" << std::endl;
   std::cout << "Average time seq = " << sum_time.second / m << " µs" << std::endl;
   std::cout << "Ratio seq/par = " << (double)sum_time.second / sum_time.first << std::endl;

}
