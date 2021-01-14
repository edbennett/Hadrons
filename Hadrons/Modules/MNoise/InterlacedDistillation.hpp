#ifndef Hadrons_MNoise_InterlacedDistillation_hpp_
#define Hadrons_MNoise_InterlacedDistillation_hpp_

#include <Hadrons/Global.hpp>
#include <Hadrons/Module.hpp>
#include <Hadrons/ModuleFactory.hpp>
#include <Hadrons/DilutedNoise.hpp>
#include <Hadrons/Modules/MDistil/Distil.hpp> // TODO: for 3D grids, shodul be handled globally

BEGIN_HADRONS_NAMESPACE

/******************************************************************************
 *                   Interlaced distillation noise module                     *
 ******************************************************************************/
BEGIN_MODULE_NAMESPACE(MNoise)

class InterlacedDistillationPar: Serializable
{
public:
    GRID_SERIALIZABLE_CLASS_MEMBERS(InterlacedDistillationPar,
                                    unsigned int, ti,
                                    unsigned int, li,
                                    unsigned int, si,
                                    unsigned int, nNoise,
                                    std::string, lapEigenPack);
};

template <typename FImpl>
class TInterlacedDistillation: public Module<InterlacedDistillationPar>
{
public:
    FERM_TYPE_ALIASES(FImpl,);
public:
    // constructor
    TInterlacedDistillation(const std::string name);
    // destructor
    virtual ~TInterlacedDistillation(void) {};
    // dependency relation
    virtual std::vector<std::string> getInput(void);
    virtual std::vector<std::string> getReference(void);
    virtual std::vector<std::string> getOutput(void);
    // setup
    virtual void setup(void);
    // execution
    virtual void execute(void);
private:
    // 3D grid, not great, needs to come from the environment
    std::unique_ptr<GridCartesian> g3d_;
};

MODULE_REGISTER_TMP(InterlacedDistillation, TInterlacedDistillation<FIMPL>, MNoise);

/******************************************************************************
 *                 TInterlacedDistillation implementation                    *
 ******************************************************************************/
// constructor /////////////////////////////////////////////////////////////////
template <typename FImpl>
TInterlacedDistillation<FImpl>::TInterlacedDistillation(const std::string name)
: Module<InterlacedDistillationPar>(name)
{
    MDistil::MakeLowerDimGrid(g3d_, envGetGrid(FermionField));
}

// dependencies/products ///////////////////////////////////////////////////////
template <typename FImpl>
std::vector<std::string> TInterlacedDistillation<FImpl>::getInput(void)
{
    std::vector<std::string> in = {};
    
    return in;
}

template <typename FImpl>
std::vector<std::string> TInterlacedDistillation<FImpl>::getReference(void)
{
    std::vector<std::string> ref = {par().lapEigenPack};

    return ref;
}

template <typename FImpl>
std::vector<std::string> TInterlacedDistillation<FImpl>::getOutput(void)
{
    std::vector<std::string> out = {getName()};
    
    return out;
}

// setup ///////////////////////////////////////////////////////////////////////
template <typename FImpl>
void TInterlacedDistillation<FImpl>::setup(void)
{
    auto &epack = envGet(typename DistillationNoise<FImpl>::LapPack, 
                         par().lapEigenPack);
    
    envCreateDerived(DistillationNoise<FImpl>, InterlacedDistillationNoise<FImpl>,
                     getName(), 1, envGetGrid(FermionField), g3d_.get(), 
                     epack, par().ti, par().li, par().si, par().nNoise);

}

// execution ///////////////////////////////////////////////////////////////////
template <typename FImpl>
void TInterlacedDistillation<FImpl>::execute(void)
{
    LOG(Message) << "Generating interlaced distillation noise with (ti, li, si) = (" 
                 << par().ti << ", " << par().li << ", " << par().si << ")"
                 << std::endl;

    auto &noise = envGetDerived(DistillationNoise<FImpl>,
                                InterlacedDistillationNoise<FImpl>, getName());

    noise.generateNoise(rngSerial());
    noise.dumpDilutionMap();
}

END_MODULE_NAMESPACE

END_HADRONS_NAMESPACE

#endif // Hadrons_MNoise_InterlacedDistillation_hpp_
