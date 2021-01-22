#ifndef Hadrons_MDistil_DistilMesonField_hpp_
#define Hadrons_MDistil_DistilMesonField_hpp_

#include <Hadrons/Global.hpp>
#include <Hadrons/Module.hpp>
#include <Hadrons/ModuleFactory.hpp>
#include <Hadrons/Modules/MDistil/Distil.hpp>
#include <Hadrons/A2AMatrix.hpp>
#include <Hadrons/DilutedNoise.hpp>

BEGIN_HADRONS_NAMESPACE

/******************************************************************************
 *                         DistilMesonField                                 *
 * Eliminates DistilVectors module. Receives LapH eigenvectors and 
 * perambulator/noise (as left/right fields). Computes MesonFields by 
 * block(and chunking it) and save them to H5 file.
 * 
 * For now, do not load anything from disk. Trying phi phi case 
 * with full-dilution.
 ******************************************************************************/

BEGIN_MODULE_NAMESPACE(MDistil)

class DistilMesonFieldMetadata: Serializable
{
public:
    GRID_SERIALIZABLE_CLASS_MEMBERS(DistilMesonFieldMetadata,
                                    std::vector<RealF>, momentum,
                                    Gamma::Algebra, gamma,
                                    std::vector<int>,   noise_pair,
                                    std::vector<std::vector<int>>, dilution_left,
                                    std::vector<std::vector<int>>, dilution_right);
};

class DistilMesonFieldPar: Serializable
{
public:
    GRID_SERIALIZABLE_CLASS_MEMBERS(DistilMesonFieldPar,
                                    std::string,    OutputStem,
                                    std::string,    MesonFieldCase,
                                    std::string,    LapEvec,
                                    std::string,    LeftInput,
                                    std::string,    RightInput,
                                    std::string,    LeftDPar,
                                    std::string,    RightDPar,
                                    std::vector<std::string>, NoisePairs,
                                    std::vector<std::string>, SourceTimesLeft,
                                    std::vector<std::string>, SourceTimesRight,
                                    std::string, Gamma,
                                    std::vector<std::string>, Momenta,
                                    int, BlockSize,
                                    int, CacheSize)
};

template <typename FImpl>
class TDistilMesonField: public Module<DistilMesonFieldPar>
{
public:
    FERM_TYPE_ALIASES(FImpl,);
public:
    // constructor
    TDistilMesonField(const std::string name);
    // destructor
    virtual ~TDistilMesonField(void) {};
    // dependency relation
    virtual std::vector<std::string> getInput(void);
    virtual std::vector<std::string> getOutput(void);
    // setup
    virtual void setup(void);
    // execution
    virtual void execute(void);
private:
    std::map<std::string,std::string>   dmf_case;
    // std::string                        momphName_;
    std::vector<Gamma::Algebra>         gamma_;
    std::vector<std::vector<RealF>>     momenta_;
    int                                 nExt_;
    int                                 nStr_;
    Vector<ComplexF>                    blockbuf_;
    Vector<Complex>                     cachebuf_;
    int                                 nt_nonzero_;
    std::vector<std::vector<int>>       noisePairs_;           // read from extermal object (diluted noise class)
    std::map<std::string, std::string>                      lrInput_;
    std::map<std::string, std::vector<std::vector<int>>>    lrSourceTimes_;
    std::string outputMFStem;
    bool                               hasPhase_{false};
};

MODULE_REGISTER_TMP(DistilMesonField, TDistilMesonField<FIMPL>, MDistil);

// aux class
template <typename FImpl>
class DMesonFieldHelper
{
private:
    int nt;
    int nd;
    std::string mfCase;
public:
    DMesonFieldHelper(int _nt , int _nd , std::string _case) : nt(_nt) , nd(_nd) , mfCase(_case)
    {
        if(mfCase=="phi phi" || mfCase=="phi rho" || mfCase=="rho phi" || mfCase=="rho rho")
        {
            LOG(Message) << "Meson field case checked: " << mfCase << std::endl;
        }
        else
        {
            HADRONS_ERROR(Argument,"Bad meson field case");
        }
    };

    int computeTimeDimension(std::vector<std::vector<int>> stl , std::vector<std::vector<int>> str)
    {
        // compute nt_nonzero_ (<nt), the number of non-zero timeslices in the final object, when there's at least one rho involved -> turn into method
        int nt_nonzero = 0;
        if(mfCase=="rho rho" || mfCase=="rho phi")
            for(auto e : stl)
                e.size() > nt_nonzero ? nt_nonzero = e.size() : 0;      //get the highest possible nt_nonzero_ from stL
        else if(mfCase=="phi rho")    //distinguishing between L and R dilution, remove if not necessary
            for(auto e : str)
                e.size() > nt_nonzero ? nt_nonzero = e.size() : 0;
        else
            nt_nonzero = nt;
        LOG(Message) << "Time dimension = " << nt_nonzero << std::endl;
        return(nt_nonzero);
    }

    std::vector<std::vector<RealF>> parseMomenta(std::vector<std::string> inputP)
    {
        std::vector<std::vector<RealF>> m;
        m.clear();
        for(auto &p_string : inputP)
        {
            auto p = strToVec<RealF>(p_string);

            if (p.size() != nd - 1)
            {
                HADRONS_ERROR(Size, "Momentum has " + std::to_string(p.size())
                                    + " components instead of " 
                                    + std::to_string(nd - 1));
            }
            m.push_back(p);
        }
        return(m);
    }

    std::vector<Gamma::Algebra> parseGamma(std::string inputG)
    {  
        std::vector<Gamma::Algebra> g;
        if (inputG == "all")
        {
            g = {
                Gamma::Algebra::Gamma5,
                Gamma::Algebra::Identity,    
                Gamma::Algebra::GammaX,
                Gamma::Algebra::GammaY,
                Gamma::Algebra::GammaZ,
                Gamma::Algebra::GammaT,
                Gamma::Algebra::GammaXGamma5,
                Gamma::Algebra::GammaYGamma5,
                Gamma::Algebra::GammaZGamma5,
                Gamma::Algebra::GammaTGamma5,
                Gamma::Algebra::SigmaXY,
                Gamma::Algebra::SigmaXZ,
                Gamma::Algebra::SigmaXT,
                Gamma::Algebra::SigmaYZ,
                Gamma::Algebra::SigmaYT,
                Gamma::Algebra::SigmaZT
            };
        }
        else
        {
            g = strToVec<Gamma::Algebra>(inputG);
        }
        return(g);
    }

    std::vector<std::vector<int>> parseNoisePairs(std::vector<std::string> inputN , std::map<std::string,std::string> caseMap , std::map<std::string,int> noiseDim )
    {
        const std::vector<std::string> lr = {"left","right"};
        std::vector<std::vector<int>> nPairs;
        nPairs.clear();
        for(auto &npair : inputN)
        {
            nPairs.push_back(strToVec<int>(npair));
            std::map<std::string, int>  noiseMapTemp = { {"left", nPairs.back()[0]} , {"right",nPairs.back()[1]} };
            for(auto &side : lr){
                if(caseMap.at(side)=="phi")    // turn this into macro?
                {
                    if( noiseMapTemp.at(side) >= noiseDim.at(side) )    // verify if input noise number is valid ( < tensor nnoise dimension)
                    {
                        HADRONS_ERROR(Size,"Noise pair element " + std::to_string(noiseMapTemp.at(side)) + "(>=" +std::to_string(noiseDim.at(side)) + ") unavailable in input tensor");
                    }
                }
                else
                {
                    if( noiseMapTemp.at(side) >= noiseDim.at(side) )
                    {
                        HADRONS_ERROR(Size,"Noise pair element " + std::to_string(noiseMapTemp.at(side)) + "(>=" +std::to_string(noiseDim.at(side)) + ") unavailable in input tensor");
                    }
                }
                if( noiseMapTemp.at(side) < 0)
                {
                    HADRONS_ERROR(Size,"Negative noise pair element");
                }
            }
        }
        return(nPairs);
    }

    void computePhase(std::vector<std::vector<RealF>> momenta_, typename FImpl::ComplexField &coor, std::vector<int> dim, std::vector<typename FImpl::ComplexField> &phase)
    {
        Complex           i(0.0,1.0);
        for (unsigned int j = 0; j < momenta_.size(); ++j)
        {
            phase[j] = Zero();
            for(unsigned int mu = 0; mu < momenta_[j].size(); mu++)
            {
                LatticeCoordinate(coor, mu);
                phase[j] = phase[j] + (momenta_[j][mu]/dim[mu])*coor;
            }
            phase[j] = exp((Real)(2*M_PI)*i*phase[j]);
        }
    }
};

/******************************************************************************
 *                 TDistilMesonField implementation                             *
 ******************************************************************************/
// constructor /////////////////////////////////////////////////////////////////
template <typename FImpl>
TDistilMesonField<FImpl>::TDistilMesonField(const std::string name)
: Module<DistilMesonFieldPar>(name)
{}

// dependencies/products ///////////////////////////////////////////////////////
template <typename FImpl>
std::vector<std::string> TDistilMesonField<FImpl>::getInput(void)
{   
    return{par().LapEvec, par().LeftInput, par().RightInput, par().LeftDPar, par().RightDPar};
}

template <typename FImpl>
std::vector<std::string> TDistilMesonField<FImpl>::getOutput(void)
{
    std::vector<std::string> out = {};
    
    return out;
}

// setup ///////////////////////////////////////////////////////////////////////
template <typename FImpl>
void TDistilMesonField<FImpl>::setup(void)
{
    GridCartesian *g     = envGetGrid(FermionField);
    GridCartesian *g3d   = envGetSliceGrid(FermionField, g->Nd() - 1);  // 3d grid (as a 4d one with collapsed time dimension)
    std::vector<std::vector<int>>       stL;      // should this come from perambulator in the future? no only if we want to use subsets of the inversions we have
    std::vector<std::vector<int>>       stR;
    const std::vector<std::string> lrPair = {"left","right"};
    // hard-coded dilution scheme (assuming dpL=dpR for now)
    const DistilParameters &dpL = envGet(DistilParameters, par().LeftDPar);
    const DistilParameters &dpR = envGet(DistilParameters, par().RightDPar);

    outputMFStem = par().OutputStem;

    DMesonFieldHelper<FImpl> helper(env().getDim(g->Nd() - 1) , g->Nd() , par().MesonFieldCase);

    dmf_case.emplace("left" , par().MesonFieldCase.substr(0,3));   //left
    dmf_case.emplace("right" , par().MesonFieldCase.substr(4,7));  //right
    
    LOG(Message) << "Selected block size: " << par().BlockSize << std::endl;
    LOG(Message) << "Selected cache size: " << par().CacheSize << std::endl;

    // parse source times l/r -> turn into method
    // outermost dimension is the time-dilution index, innermost one are the non-zero source timeslices
    // in phi phi, save all timeslices, but in the other cases save only the non-zero  ones...
    stL.clear();
    for(auto &stl : par().SourceTimesLeft)
        stL.push_back(strToVec<int>(stl));
    stR.clear();
    for(auto &str : par().SourceTimesRight)
        stR.push_back(strToVec<int>(str));

    nt_nonzero_ = helper.computeTimeDimension(stL,stR);
    
    // populate lrInput_ and lrSourceTimes_
    lrInput_       = {{"left",par().LeftInput},{"right",par().RightInput}};
    lrSourceTimes_ = {{"left",stL},{"right",stR}};  

    // parse and validate input
    std::map<std::string, int> noiseDimension = {{"left",0},{"right",0}}; ;
    for(auto & side : lrPair)
    {
        if(dmf_case.at(side)=="phi")
        {
            auto &inTensor = envGet(PerambTensor , lrInput_.at(side));
            noiseDimension.at(side) = inTensor.tensor.dimensions().at(3);
        }
        else
        {
            auto &inTensor = envGet(NoiseTensor , lrInput_.at(side));
            noiseDimension.at(side) = inTensor.tensor.dimensions().at(3);
        }
    }
    noisePairs_ = helper.parseNoisePairs(par().NoisePairs , dmf_case , noiseDimension);
    
    // momenta and gamma parse -> turn into method
    momenta_ = helper.parseMomenta(par().Momenta);
    gamma_ = helper.parseGamma(par().Gamma);
    nExt_ = momenta_.size(); //noise pairs computed independently, but can optmize embedding it into nExt??
    nStr_ = gamma_.size();
    
    //populate matrix sets
    blockbuf_.resize(nExt_*nStr_*nt_nonzero_*par().BlockSize*par().BlockSize);
    cachebuf_.resize(nExt_*nStr_*env().getDim(g->Nd() - 1)*par().CacheSize*par().CacheSize);
    
    envTmp(FermionField,                    "fermion3dtmp",         1, g3d);
    envTmp(ColourVectorField,               "fermion3dtmp_nospin",  1, g3d);
    envTmp(ColourVectorField,               "evec3d",               1, g3d);
    envTmp(std::vector<FermionField>,       "left",                 1, stL.size()*dpL.LI*dpL.SI, g);
    envTmp(std::vector<FermionField>,       "right",                1, stL.size()*dpR.LI*dpR.SI, g);
    envTmpLat(ComplexField, "coor");
    envCache(std::vector<ComplexField>,     "phasename",            1, momenta_.size(), g);
    envTmpLat(FermionField,                 "fermion4dtmp");
}

// execution ///////////////////////////////////////////////////////////////////
template <typename FImpl>
void TDistilMesonField<FImpl>::execute(void)
{
    // temps
    envGetTmp(FermionField,                 fermion3dtmp);
    envGetTmp(ColourVectorField,            fermion3dtmp_nospin);
    envGetTmp(FermionField,                 fermion4dtmp);
    envGetTmp(ColourVectorField,            evec3d);
    envGetTmp(std::vector<FermionField>,    left);
    envGetTmp(std::vector<FermionField>,    right);

    int blockSize_ = par().BlockSize;
    int cacheSize_ = par().CacheSize;
    int vol = env().getGrid()->_gsites;
    const unsigned int nd = env().getGrid()->Nd();
    const int nt = env().getDim(nd - 1);
    const int Ntlocal = env().getGrid()->LocalDimensions()[nd - 1];
    const int Ntfirst = env().getGrid()->LocalStarts()[nd - 1];
    DMesonFieldHelper<FImpl> helper(nt , nd , par().MesonFieldCase);

    // hard-coded dilution schem &  assuming dilution_left == dilution_right; other cases?...
    // replace by noise class
    const DistilParameters &dpL = envGet(DistilParameters, par().LeftDPar);
    const DistilParameters &dpR = envGet(DistilParameters, par().RightDPar);

    int nInversions             = MIN(dpL.inversions,dpR.inversions);   // when different dilution schemes are useful? here I'm taking just the 'smaller'
    assert(nInversions >= lrSourceTimes_.at("left").size());    // should nInversions be greater or equal to SourceTimesLeft.size() , SourceTimesRight.size() always? I guess so
    assert(nInversions >= lrSourceTimes_.at("right").size());

    auto &epack = envGet(LapEvecs, par().LapEvec);
    auto &phase = envGet(std::vector<ComplexField>, "phasename");

    //compute momentum phase
    if (!hasPhase_)
    {
        startTimer("momentum phases");
        envGetTmp(ComplexField, coor);
        helper.computePhase(momenta_, coor, env().getDim(), phase);
        hasPhase_ = true;
        stopTimer("momentum phases");
    }

    //clean this out
    int LI = dpL.LI , SI = dpL.SI;
    int dilutionSize_LS = LI * SI;

    int nVec        = dpL.nvec;
    int nNoiseLeft  = dpL.nnoise;
    int nNoiseRight = dpR.nnoise;

    // do not use operator []!! similar but better way to do that? maybe map of pointers?
    std::map<std::string, std::vector<FermionField>&>       lrDistVector  = {{"left",left},{"right",right}};
    const std::vector<std::string> lrPair = {"left","right"};

    long    global_counter = 0;
    double  global_flops = 0.0;
    double  global_bytes = 0.0;

    for(auto &inoise : noisePairs_)
    {
        //set up io object and metadata for all gamma/momenta
        std::vector<A2AMatrixIo<ComplexF>> matrixIoTable;
        DistilMesonFieldMetadata md;
        for(int iExt=0; iExt<nExt_; iExt++)
        for(int iStr=0; iStr<nStr_; iStr++)
        {
            // metadata;
            md.momentum = momenta_[iExt];
            md.gamma = gamma_[iStr];
            md.noise_pair = inoise;
            md.dilution_left = lrSourceTimes_.at("left");
            md.dilution_right = lrSourceTimes_.at("right");

            std::stringstream ss;
            ss << md.gamma << "_";
            for (unsigned int mu = 0; mu < md.momentum.size(); ++mu)
                ss << md.momentum[mu] << ((mu == md.momentum.size() - 1) ? "" : "_");
            std::string groupName = ss.str();

            //init file here (do not create dataset yet)
            //IO configuration for fixed test gamma and momentum
            
            std::string outputStem = outputMFStem + "/noise" + std::to_string(inoise[0]) + "_" + std::to_string(inoise[1]) + "/";
            Hadrons::mkdir(outputStem);
            std::string mfName = groupName+"_"+dmf_case.at("left")+"-"+dmf_case.at("right")+".h5";
            A2AMatrixIo<ComplexF> matrixIo(outputStem+mfName, groupName, nt_nonzero_, dilutionSize_LS, dilutionSize_LS);  // automatise name choice according to momenta_ and gamma_
            matrixIoTable.push_back(matrixIo);

            //initialize file with no outputName group (containing atributes of momentum and gamma) but no dataset inside
            if(env().getGrid()->IsBoss())
            {
                startTimer("IO: total");
                startTimer("IO: file creation");
                matrixIoTable.back().initFile(md);
                stopTimer("IO: file creation");
                stopTimer("IO: total");
            }
        }

        std::map<std::string,int&> lfNoise = {{"left",inoise[0]},{"right",inoise[1]}}; //do not use []

        LOG(Message) << "Noise pair: " << inoise << std::endl;

        LOG(Message) << "Gamma:" << std::endl;
        LOG(Message) << gamma_ << std::endl;
        LOG(Message) << "Momenta:" << std::endl;
        LOG(Message) << momenta_ << std::endl;

        // computation, still ignoring gamma5 hermiticity
        for(auto &side : lrPair)
        {
            //each dtL, dtR pair corresponds to a different dataset here
            for (int dt = 0; dt<lrSourceTimes_.at(side).size(); dt++)         //loop over time dilution index
            {
                // computation of phi or rho
                for(int iD=0 ; iD<dilutionSize_LS ; iD++)
                {
                    int dk = iD%LI;
                    int ds = (iD - dk)/LI;
                    lrDistVector.at(side)[dt*dilutionSize_LS + iD] = Zero();
                    if(dmf_case.at(side)=="phi")
                    {
                        auto &inTensor = envGet(PerambTensor , lrInput_.at(side));
                        for (int t = Ntfirst; t < Ntfirst + Ntlocal; t++)   //loop over (local) timeslices
                        {
                            fermion3dtmp = Zero();
                            for (int k = 0; k < nVec; k++)
                            {
                                ExtractSliceLocal(evec3d,epack.evec[k],0,t-Ntfirst,nd - 1);
                                fermion3dtmp += evec3d * inTensor.tensor(t, k, dk, lfNoise.at(side), dt, ds);
                            }
                            InsertSliceLocal(fermion3dtmp,lrDistVector.at(side)[dt*dilutionSize_LS + iD],0,t-Ntfirst,nd - 1);
                        }
                    }
                    else if(dmf_case.at(side)=="rho"){
                        auto &inTensor = envGet(NoiseTensor , lrInput_.at(side));
                        for(int t : lrSourceTimes_.at(side)[dt])
                        {
                            if (t >= Ntfirst && t < Ntfirst + Ntlocal)
                            {
                                for (int k = dk; k < nVec; k += LI)
                                {
                                    for (int s = ds; s < Ns; s += SI)
                                    {
                                        ExtractSliceLocal(evec3d,epack.evec[k],0,t-Ntfirst,nd - 1);
                                        fermion3dtmp_nospin = evec3d * inTensor.tensor(lfNoise.at(side), t, k , s);
                                        fermion3dtmp = Zero();
                                        pokeSpin(fermion3dtmp,fermion3dtmp_nospin,s);
                                        fermion4dtmp = Zero();
                                        InsertSliceLocal(fermion3dtmp,fermion4dtmp,0,t-Ntfirst,nd - 1);
                                        lrDistVector.at(side)[dt*dilutionSize_LS + iD] += fermion4dtmp;
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }

        // computing mesonfield blocks and saving to disk
        for (int dtL = 0; dtL < lrSourceTimes_.at("left").size() ; dtL++)
        for (int dtR = 0; dtR < lrSourceTimes_.at("right").size() ; dtR++)
        {
            if(!(par().MesonFieldCase=="rho rho" && dtL!=dtR))
            {
                std::string datasetName = "dtL"+std::to_string(dtL)+"_dtR"+std::to_string(dtR);
                LOG(Message) << "- Computing dilution dataset " << datasetName << "..." << std::endl;

                // int nblocki = left.size()/blockSize_ + (((left.size() % blockSize_) != 0) ? 1 : 0);
                // int nblockj = right.size()/blockSize_ + (((right.size() % blockSize_) != 0) ? 1 : 0);
                int nblocki = dilutionSize_LS/blockSize_ + (((dilutionSize_LS % blockSize_) != 0) ? 1 : 0);
                int nblockj = dilutionSize_LS/blockSize_ + (((dilutionSize_LS % blockSize_) != 0) ? 1 : 0);

                // loop over blocks in the current time-dilution block
                for(int iblock=0 ; iblock<dilutionSize_LS ; iblock+=blockSize_) //set according to memory size
                for(int jblock=0 ; jblock<dilutionSize_LS ; jblock+=blockSize_)
                {
                    int iblockSize = MIN(dilutionSize_LS-iblock,blockSize_);    // iblockSize is the size of the current block (indexed by i); N_i-i is the size of the eventual remainder block
                    int jblockSize = MIN(dilutionSize_LS-jblock,blockSize_);
                    A2AMatrixSet<ComplexF> block(blockbuf_.data(), nExt_ , nStr_ , nt_nonzero_, iblockSize, jblockSize);

                    LOG(Message) << "Distil matrix block " 
                    << jblock/blockSize_ + nblocki*iblock/blockSize_ + 1 
                    << "/" << nblocki*nblockj << " [" << iblock << " .. " 
                    << iblock+iblockSize-1 << ", " << jblock << " .. " << jblock+jblockSize-1 << "]" 
                    << std::endl;

                    // LOG(Message) << "Block size = "         << nt_nonzero_*iblockSize*jblockSize*sizeof(ComplexF) << "MB/momentum/gamma" << std::endl;
                        // LOG(Message) << "Cache blocks size = "   << nt*cacheSize_*cacheSize_*sizeof(ComplexD) << "MB/momentum/gamma" << std::endl;  //remember to change this in case I change chunk size from nt to something else

                    double flops       = 0.0;
                    double bytes       = 0.0;
                    double time_kernel = 0.0;
                    double nodes    = env().getGrid()->NodeCount();

                    // loop over cache_ blocks in the current block
                    for(int icache=0 ; icache<iblockSize ; icache+=cacheSize_)   //set according to cache_ size
                    for(int jcache=0 ; jcache<jblockSize ; jcache+=cacheSize_)
                    {
                        int icacheSize = MIN(iblockSize-icache,cacheSize_);      // icacheSize is the size of the current cache_ block (indexed by ii); N_ii-ii is the size of the remainder cache_ block
                        int jcacheSize = MIN(jblockSize-jcache,cacheSize_);
                        A2AMatrixSet<Complex> blockCache(cachebuf_.data(), nExt_, nStr_, nt, icacheSize, jcacheSize);

                        double timer = 0.0;
                        startTimer("kernel");
                        A2Autils<FIMPL>::MesonField(blockCache, &left[dtL*dilutionSize_LS+iblock+icache], &right[dtR*dilutionSize_LS+jblock+jcache], gamma_, phase, nd - 1, &timer);
                        stopTimer("kernel");

                        time_kernel += timer;
                        
                        // nExt is currently # of momenta , nStr is # of gamma matrices
                        flops += vol*(2*8.0+6.0+8.0*nExt_)*icacheSize*jcacheSize*nStr_;
                        bytes += vol*(12.0*sizeof(ComplexD))*icacheSize*jcacheSize
                              +  vol*(2.0*sizeof(ComplexD)*nExt_)*icacheSize*jcacheSize*nStr_;

                        // std::cout<< "block dimensions " << block.dimensions().at(2) << std::endl;
                        // std::cout<< "blockcache dimensions " << blockCache.dimensions().at(2) << std::endl << std::cin.get();

                        // loop through the cacheblock (inside them) and point blockCache to block
                        startTimer("cache copy");
                        if(par().MesonFieldCase=="phi phi")
                        {
                            thread_for_collapse( 5, iExt ,nExt_,{
                            for(int iStr=0 ;iStr<nStr_ ; iStr++)
                            for(int t=0 ; t<nt ; t++)
                            for(int iicache=0 ; iicache<icacheSize ; iicache++)
                            for(int jjcache=0;  jjcache<jcacheSize ; jjcache++)
                                block(iExt,iStr,t,icache+iicache,jcache+jjcache) = blockCache(iExt,iStr,t,iicache,jjcache);
                            });
                        }
                        else
                        {
                            // std::cout << lrSourceTimes_.at("left") << std::cin.get();
                            thread_for_collapse( 5, iExt ,nExt_,{
                            // for ( uint64_t iExt=0;iExt<nExt_;iExt++) { 
                            for(int iStr=0 ;iStr<nStr_ ; iStr++)
                            for(int it=0 ; it<nt_nonzero_ ; it++)  //only wish to copy non-zero timeslices to block
                            for(int iicache=0 ; iicache<icacheSize ; iicache++)
                            for(int jjcache=0;  jjcache<jcacheSize ; jjcache++)
                                block(iExt,iStr,it,icache+iicache,jcache+jjcache) = blockCache(iExt,iStr,lrSourceTimes_.at("left")[dtL][it],iicache,jjcache);
                            // }
                            });
                        }
                        stopTimer("cache copy");
                    }

                    LOG(Message) << "Kernel perf (flops) " << flops/time_kernel/1.0e3/nodes 
                                << " Gflop/s/node " << std::endl;
                    LOG(Message) << "Kernel perf (read) " << bytes/time_kernel*0.000931322574615478515625/nodes //  1.0e6/1024/1024/1024/nodes
                                << " GB/s/node "  << std::endl;
                    global_counter++;
                    global_flops += flops/time_kernel/1.0e3/nodes ;
                    global_bytes += bytes/time_kernel*0.000931322574615478515625/nodes ; // 1.0e6/1024/1024/1024/nodes

                    // saving current block to disk
                    LOG(Message) << "Writing block to disk" << std::endl;
                    startTimer("IO: total");
                    startTimer("IO: write block");
                    double ioTime = -getDTimer("IO: write block");
#ifdef HADRONS_A2AM_PARALLEL_IO
                    //parallel io
                    int inode = env().getGrid()->ThisRank();
                    int nnode = env().getGrid()->RankCount(); 
                    LOG(Message) << "Starting parallel IO. Rank count=" << nnode  << std::endl;
                    env().getGrid()->Barrier();
                    for(int ies=inode ; ies<nExt_*nStr_ ; ies+=nnode){
                        int iExt = ies/nStr_;
                        int iStr = ies%nStr_;
                        if(iblock==0 && jblock==0){              // creates dataset only if it's the first block of the dataset
                            matrixIoTable[iStr + nStr_*iExt].saveBlock(block, iExt , iStr , iblock, jblock, datasetName, cacheSize_);   //set surface chunk size as cacheSize_ (the chunk itself is 3D)
                        }
                        else{
                            matrixIoTable[iStr + nStr_*iExt].saveBlock(block, iExt , iStr , iblock, jblock, datasetName);
                        }
                    }
                    env().getGrid()->Barrier();
#else
                    // serial io, can remove later
                    LOG(Message) << "Starting serial IO" << std::endl;
                    for(int iExt=0; iExt<nExt_; iExt++)
                    for(int iStr=0; iStr<nStr_; iStr++)
                    {
                        if(iblock==0 && jblock==0){              // creates dataset only if it's the first block of the dataset
                            matrixIoTable[iStr + nStr_*iExt].saveBlock(block, iExt, iStr, iblock, jblock, datasetName, cacheSize_);   //set surface chunk size as cacheSize_ (the chunk itself is 3D)
                        }
                        else{
                            matrixIoTable[iStr + nStr_*iExt].saveBlock(block, iExt, iStr, iblock, jblock, datasetName);
                        }
                    }
#endif
                    stopTimer("IO: total");
                    stopTimer("IO: write block");
                    ioTime    += getDTimer("IO: write block");
                    int bytesBlockSize  = static_cast<double>(nExt_*nStr_*nt_nonzero_*iblockSize*jblockSize*sizeof(ComplexF));
                    LOG(Message)    << "HDF5 IO done " << sizeString(bytesBlockSize) << " in "
                                    << ioTime  << " us (" 
                                    << bytesBlockSize/ioTime*0.95367431640625 // 1.0e6/1024/1024
                                    << " MB/s)" << std::endl;
                }
            }
        }
        LOG(Message) << "Meson fields saved at " << outputMFStem << std::endl;
    }
    LOG(Message) << "MesonField kernel executed " << global_counter << " times on " << cacheSize_ << "^2 cache blocks" << std::endl;
    LOG(Message) << "Average kernel perf (flops) " << global_flops/global_counter << " Gflop/s/node " << std::endl;
    LOG(Message) << "Average kernel perf (read) " << global_bytes/global_counter  << " GB/s/node "  << std::endl;
}


END_MODULE_NAMESPACE

END_HADRONS_NAMESPACE

#endif // Hadrons_MDistil_DistilMesonField_hpp_
