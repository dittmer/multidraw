#ifndef multidraw_Task_h
#define multidraw_Task_h

#include <vector>
#include <memory>
#include <condition_variable>

#include <boost/lockfree/queue.hpp>

namespace multidraw {

  class ExprFiller;
  class Cut;

  class TaskBase {
  public:
    TaskBase(std::condition_variable&);
    virtual ~TaskBase() {}

    virtual void execute() = 0;

  protected:
    std::condition_variable& condition_;
  };

  typedef boost::lockfree::queue<TaskBase*> TaskQueue;

  class FillerTask {
  public:
    FillerTask(std::function<void(std::vector<double> const&)> const& fillExpr, std::vector<double> const& eventWeights, std::condition_variable&);

    void execute() override;

  private:
    std::function<void(std::vector<double> const&)> fillExpr_;
    std::vector<double> const& eventWeights_;
  };

  class CutTask {
  public:
    CutTask(Cut&, std::vector<double> const& eventWeights, TaskQueue& queue, std::condition_variable&);

    void execute() override;
    void executeFillers();

  private:
    Cut& cut_;
    std::vector<double> const& eventWeights_;

    std::vector<FillerTask*> fillerTasks_;
    TaskQueue& queue_;
  };

}

#endif
