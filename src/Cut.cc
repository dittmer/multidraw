#include "../interface/Cut.h"
#include "../interface/ExprFiller.h"

#include "TTreeFormulaManager.h"

#include <iostream>

multidraw::Cut::Cut(char const* _name, char const* _expr/* = ""*/) :
  name_(_name),
  cutExpr_(_expr)
{
}

multidraw::Cut::~Cut()
{
  unlinkTree();

  for (auto* filler : fillers_) {
    filler->mergeBack();
    delete filler;
  }
}

TString
multidraw::Cut::getName() const
{
  if (name_.Length() == 0)
    return "[Event filter]";
  else
    return name_;
}

void
multidraw::Cut::setPrintLevel(int _l)
{
  printLevel_ = _l;
  for (auto* filler : fillers_)
    filler->setPrintLevel(_l);
}

void
multidraw::Cut::bindTree(FormulaLibrary& _library)
{
  counter_ = 0;

  compiledCut_.reset();
  if (cutExpr_.Length() != 0)
    compiledCut_ = _library.getFormula(cutExpr_);

  for (auto* filler : fillers_)
    filler->bindTree(_library);
}

void
multidraw::Cut::unlinkTree()
{
  compiledCut_.reset();
  delete instanceMask_;
  instanceMask_ = nullptr;

  for (auto* filler : fillers_)
    filler->unlinkTree();
}

multidraw::Cut*
multidraw::Cut::threadClone(FormulaLibrary& _library) const
{
  Cut* clone(new Cut(name_, cutExpr_));
  clone->setPrintLevel(-1);

  for (auto* filler : fillers_)
    clone->addFiller(*filler->threadClone(_library));

  clone->bindTree(_library);

  return clone;
}

bool
multidraw::Cut::cutDependsOn(TTree const* _tree) const
{
  if (!compiledCut_)
    return false;

  unsigned iL(0);
  while (true) {
    auto* leaf(compiledCut_->GetLeaf(iL++));
    if (leaf == nullptr)
      return false;

    if (leaf->GetBranch()->GetTree() == _tree)
      return true;
  }

  // won't reach here
  return false;
}

void
multidraw::Cut::initialize()
{
  delete instanceMask_;
  instanceMask_ = nullptr;

  if (!compiledCut_)
    return;

  // Check if any formula belonging to this cut already has a manager
  auto* formulaManager(compiledCut_->GetManager());

  if (formulaManager == nullptr) {
    for (auto* filler : fillers_) {
      for (unsigned iD(0); iD != filler->getNdim(); ++iD) {
        formulaManager = filler->getFormula(iD)->GetManager();
        if (formulaManager != nullptr)
          break;
      }
      if (formulaManager != nullptr)
        break;

      auto* reweight(filler->getReweight());
      for (unsigned iD(0); iD != reweight->getNdim(); ++iD) {
        formulaManager = reweight->getFormula(iD)->GetManager();
        if (formulaManager != nullptr)
          break;
      }
      if (formulaManager != nullptr)
        break;
    }
  }

  if (formulaManager == nullptr) {
    // If none has a manager, create new. Manager will be deleted by the last TTreeFormula
    formulaManager = new TTreeFormulaManager;
  }

  formulaManager->Add(compiledCut_.get());

  for (auto* filler : fillers_) {
    for (unsigned iD(0); iD != filler->getNdim(); ++iD)
      formulaManager->Add(filler->getFormula(iD));

    auto* reweight(filler->getReweight());
    for (unsigned iD(0); iD != reweight->getNdim(); ++iD)
      formulaManager->Add(reweight->getFormula(iD));
  }

  formulaManager->Sync();

  if (formulaManager->GetMultiplicity() > 0)
    instanceMask_ = new std::vector<bool>;
}

bool
multidraw::Cut::evaluate()
{
  if (!compiledCut_)
    return true;

  unsigned nD(compiledCut_->GetManager()->GetNdata());

  if (instanceMask_ != nullptr)
    instanceMask_->assign(nD, false);

  if (printLevel_ > 2)
    std::cout << "        " << getName() << " has " << nD << " iterations" << std::endl;

  bool any(false);

  for (unsigned iD(0); iD != nD; ++iD) {
    if (compiledCut_->EvalInstance(iD) == 0.)
      continue;

    any = true;

    if (printLevel_ > 2)
      std::cout << "        " << getName() << " iteration " << iD << " pass" << std::endl;

    if (instanceMask_ != nullptr)
      (*instanceMask_)[iD] = true;
  }
  
  return any;
}

void
multidraw::Cut::fillExprs(std::vector<double> const& _eventWeights)
{
  ++counter_;

  for (auto* filler : fillers_)
    filler->fill(_eventWeights, instanceMask_);
}
