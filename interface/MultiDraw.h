#ifndef multidraw_MultiDraw_h
#define multidraw_MultiDraw_h

#include "TTreeFormulaCached.h"
#include "ExprFiller.h"
#include "Plot1DFiller.h"
#include "Plot2DFiller.h"
#include "TreeFiller.h"
#include "Cut.h"
#include "Evaluable.h"
#include "FormulaLibrary.h"

#include "TChain.h"
#include "TH1.h"
#include "TH2.h"
#include "TTree.h"
#include "TString.h"

#include <vector>

namespace multidraw {

  //! A handy class to fill multiple histograms in one pass over a TChain using string expressions.
  /*!
   * Usage
   *  MultiDraw drawer;
   *  drawer.addInputPath("source.root");
   *  drawer.addCut("baseline", "electrons.loose && TMath::Abs(electrons.eta_) < 1.5");
   *  drawer.addCut("fullsel", "electrons.tight");
   *  TH1D* h1 = new TH1D("h1", "histogram 1", 100, 0., 100.);
   *  TH1D* h2 = new TH1D("h2", "histogram 2", 100, 0., 100.);
   *  drawer.addPlot(h1, "electrons.pt_", "", "baseline");
   *  drawer.addPlot(h2, "electrons.pt_", "", "fullsel");
   *  drawer.fillPlots();
   *
   * In the above example, histogram h1 is filled with the pt of the loose barrel electrons, and
   * h2 with the subset of such electrons that also pass the tight selection.
   * It is also possible to set cuts and reweights for individual plots. Event-wide weights can
   * be set by three methods setWeightBranch, setConstantWeight, and setGlobalReweight.
   */
  class MultiDraw {
  public:
    //! Constructor.
    /*!
     * \param treeName  Name of the input tree.
     */
    MultiDraw(char const* treeName = "events");
    ~MultiDraw();

    //! Add an input file.
    void addInputPath(char const* path) { tree_.Add(path); }
    //! Add a friend tree (not tested)
    void addFriend(char const* treeName, TObjArray const* paths);
    //! Apply an entry list.
    void applyEntryList(TEntryList* elist) { tree_.SetEntryList(elist); }
    //! Set the name and the C variable type of the weight branch. Pass an empty string to unset.
    void setWeightBranch(char const* bname) { weightBranchName_ = bname; }
    //! Set a global filtering cut. Events not passing this expression are not considered at all.
    void setFilter(char const* expr);
    //! Add a new cut.
    void addCut(char const* name, char const* expr);
    //! Remove a cut.
    void removeCut(char const* name);
    //! Apply a constant weight (e.g. luminosity times cross section) to all events, possibly varying by input tree.
    /*
     * Tree number option is useful when e.g. running over multiple MC samples with different cross
     * sections in one shot.
     * When tree number >= 0 and exclusive = true, the global weight is not applied to the events
     * from the given tree number. If exclusive = false, the tree-by-tree weight is applied on top
     * of the global one.
     */
    void setConstantWeight(double w, int treeNumber = -1, bool exclusive = true);
    //! Set an overall reweight, possibly varying by input tree.
    /*!
     * Reweight factor can be set in two ways. If the second argument is nullptr,
     * the value of expr in every event is used as the weight. If instead a TH1,
     * TGraph, or TF1 is passed as the second argument, the value of expr is used
     * to look up the y value of the source object, which is used as the weight.
     * Tree number option is useful when e.g. running over multiple MC samples with different cross
     * sections in one shot.
     */
    void setReweight(char const* expr, TObject const* source = nullptr, int treeNumber = -1, bool exclusive = true);
    //! Set a prescale factor
    /*!
     * When prescale_ > 1, only events that satisfy eventNumber % prescale_ == 0 are read.
     * If evtNumBranch is set, branch of the given name is read as the eventNumber. The
     * tree entry number is used otherwise.
     */
    void setPrescale(unsigned p, char const* evtNumBranch = "");
    //! Add a 1D histogram to fill.
    Plot1DFiller& addPlot(TH1* hist, char const* expr, char const* cutName = "", char const* reweight = "", Plot1DFiller::OverflowMode mode = Plot1DFiller::kDefault);
    //! Add a 2D histogram to fill.
    Plot2DFiller& addPlot2D(TH2* hist, char const* xexpr, char const* yexpr, char const* cutName = "", char const* reweight = "");
    //! Add a tree to fill.
    TreeFiller& addTree(TTree* tree, char const* cutName = "", char const* reweight = "");

    //! Run and fill the plots and trees.
    void execute(long nEntries = -1, long firstEntry = 0);

    //! Set the number of threds.
    void setNumThreads(unsigned n) { numThreads_ = n; }

    //! Set the print level.
    /*
     * Level -1: silent
     * Level  0: print number of events every 10000
     * Level  1: additionally print the summary of execution
     * Level  2: display info about each added plot/tree and each new file
     * Level  3: print every 100 events
     * Level  4: debug print out
     */
    void setPrintLevel(int l) { printLevel_ = l; }
    void setDoTimeProfile(bool d) { doTimeProfile_ = d; }

    long getTotalEvents() const { return totalEvents_; }

    unsigned numObjs() const;

    TChain const& getTree() const { return tree_; }

  private:
    //! Handle addPlot and addTree with the same interface (requires a callback to generate the right object)
    typedef std::function<ExprFiller*(TTreeFormulaCachedPtr const&)> ObjGen;
    ExprFiller& addObj_(TString const& cutName, char const* reweight, ObjGen const&);
    Cut& findCut_(TString const& cutName) const;

    TChain tree_;
    std::vector<TChain*> friendTrees_{};
    TString weightBranchName_{"weight"};
    TString evtNumBranchName_{""};

    unsigned prescale_{1};

    std::vector<Cut*> cuts_;
    FormulaLibrary library_;

    double globalWeight_{1.};
    Evaluable globalReweight_{};
    std::map<int, std::pair<double, bool>> treeWeight_{};
    std::map<int, std::pair<Evaluable, bool>> treeReweight_{};

    unsigned numThreads_{1};
    int printLevel_{0};
    bool doTimeProfile_{0};

    long totalEvents_{0};
  };

}

#endif
