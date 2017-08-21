#include <chrono>

namespace vtkMapType
{

class Timer
{
public:
  Timer() { reset(); }

  ~Timer() = default;

  void reset() { timestamp = std::chrono::high_resolution_clock::now(); }

  template <typename Period>
  size_t elapsed()
  {
    const auto current = std::chrono::high_resolution_clock::now();
    const auto delta = std::chrono::duration_cast<Period>(current - timestamp);

    return static_cast<size_t>(delta.count());
  }

private:
  Timer(const Timer&) = delete;
  void operator=(const Timer&) = delete;

  std::chrono::high_resolution_clock::time_point timestamp;
};
}
