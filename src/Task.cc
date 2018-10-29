#include "../interface/Task.h"
#include "../interface/ExprFiller.h"
#include "../interface/Cut.h"

multidraw::TaskBase::TaskBase(std::condition_variable& _condition) :
  condition_(_condition)
{
}

multidraw::FillerTask::FillerTask(std::function<void(std::vector<double> const&)> const& _fillExpr, std::vector<double> const& _eventWeights, std::condition_variable& _condition) :
  TaskBase(_condition),
  fillExpr_(_fillExpr),
  eventWeights_(_eventWeights)
{
}

void
multidraw::FillerTask::execute()
{
  fillExpr_(eventWeights_);
}

multidraw::CutTask::CutTask(Cut& _cut, std::vector<double> const& _eventWeights, TaskQueue& _queue, std::condition_variable& _condition) :
  TaskBase(_condition),
  cut_(_cut),
  eventWeights_(_eventWeights),
  queue_(_queue)
{
  for (unsigned iF(0); iF != cut_.getNFillers(); ++iF)
    fillerTasks_.push_back(new FillerTask(cut_.getFillExpr(iF), _eventWeights));
}

void
multidraw::CutTask::execute()
{
  if (cut_->evaluate()) {
    for (auto* t : fillerTasks_)
      queue_.push(t);
  }
}

void
multidraw::CutTask::executeFillers()
{
  for (auto* t : fillerTasks_)
    queue_.push(t);
}
