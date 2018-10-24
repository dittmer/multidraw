#include "../interface/Task.h"
#include "../interface/ExprFiller.h"
#include "../interface/Cut.h"

multidraw::FillerTask::FillerTask(ExprFiller& _filler, std::vector<double> const& _eventWeights, std::vector<bool>* const& _mask) :
  filler_(_filler),
  eventWeights_(_eventWeights),
  mask_(_mask)
{
}

void
multidraw::FillerTask::execute()
{
  filler_.fill(eventWeights_, mask_);
}

multidraw::CutTask::CutTask(Cut& _cut, std::vector<double> const& _eventWeights, TaskQueue& _queue) :
  cut_(_cut),
  eventWeights_(_eventWeights),
  queue_(_queue)
{
  for (unsigned iF(0); iF != cut_.getNFillers(); ++iF)
    fillerTasks_.push_back(cut_.makeFillerTask(iF, _eventWeights));
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
