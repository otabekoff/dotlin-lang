#include <chrono>
#include <iostream>
#include <string>

long currentTimeMillis() {
  using namespace std::chrono;
  return duration_cast<milliseconds>(system_clock::now().time_since_epoch())
      .count();
}

int main() {
  std::string a = "global";
  {
    std::string b = "first";
    {
      std::string c = "second";
      {
        std::string d = "third";
        {
          std::string e = "fourth";
          {
            std::string f = "fifth";
            // Access variables from different depths 1,000,000 times
            long start = currentTimeMillis();
            int i = 0;
            while (i < 1000000) {
              std::string x = a;
              std::string y = b;
              std::string z = e;
              i = i + 1;
            }
            long end = currentTimeMillis();
            std::cout << "Time taken: " << (end - start) << "ms" << std::endl;
          }
        }
      }
    }
  }
  return 0;
}
