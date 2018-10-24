#ifndef multidraw_Task_h
#define multidraw_Task_h

#include <vector>
#include <memory>

#include <boost/lockfree/queue.hpp>

namespace multidraw {

  class ExprFiller;
  class Cut;

  class TaskBase {
  public:
    TaskBase() {}
    virtual ~TaskBase() {}

    virtual void execute() = 0;
  };

  typedef boost::lockfree::queue<TaskBase*> TaskQueue;

  class FillerTask {
  public:
    FillerTask(ExprFiller&, std::vector<double> const& eventWeights, std::vector<bool>* const& mask);

    void execute() override;

  private:
    ExprFiller& filler_;
    std::vector<double> const& eventWeights_;
    std::vector<bool>* const& mask_;
  };

  class CutTask {
  public:
    CutTask(Cut&, std::vector<double> const& eventWeights, TaskQueue& queue);

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
