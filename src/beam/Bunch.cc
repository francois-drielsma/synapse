#include "Bunch.hh"

namespace Beam {

Bunch::Bunch() :
  _name(""), _dim(0), _pos(0.), _mass(0.), _fraction(0.), _data(), _core(), _alpha(), _beta(),
  _gamma(), _mecl(), _eps(), _neps(), _seps(), _feps(), _qamp(), _amps(), _vols(), _de() {
}

Bunch::Bunch(const BunchMap& samples,
	     const double pos,
	     const std::string& name,
	     const double mass) :
  _name(name), _dim(0), _pos(pos), _mass(mass), _fraction(0.), _data(), _core(), _alpha(), _beta(),
  _gamma(), _mecl(), _eps(), _neps(), _seps(), _feps(), _qamp(), _amps(), _vols(), _de() {


  BunchMap errors;
  try {
    Initialize(samples, errors);
  } catch ( Exceptions::Exception& e ) {
    throw(Exceptions::Exception(Exceptions::nonRecoverable,
	  "Could not initialize"+std::string(e.what()),
	  "Bunch::Bunch"));
  }
}

Bunch::Bunch(const BunchMap& samples,
  	     const BunchMap& errors,
	     const double pos,
	     const std::string& name,
	     const double mass) :
  _name(name), _dim(0), _pos(pos), _mass(mass), _fraction(0.), _data(), _core(), _alpha(), _beta(),
  _gamma(), _mecl(), _eps(), _neps(), _seps(), _feps(), _qamp(), _amps(), _vols(), _de() {

  try {
    Initialize(samples, errors);
  } catch ( Exceptions::Exception& e ) {
    throw(Exceptions::Exception(Exceptions::nonRecoverable,
	  "Could not initialize"+std::string(e.what()),
	  "Bunch::Bunch"));
  }
}

Bunch::Bunch(const Bunch& bunch) {
  *this = bunch;
}

Bunch& Bunch::operator=(const Bunch& bunch) {
  if ( this == &bunch )
      return *this;

  _name = bunch._name;
  _dim = bunch._dim;
  _pos = bunch._pos;
  _mass = bunch._mass;
  _fraction = bunch._fraction;
  _data = bunch._data;
  _core = bunch._core;
  _alpha = bunch._alpha;
  _beta = bunch._beta;
  _gamma = bunch._gamma;
  _mecl = bunch._mecl;
  _eps = bunch._eps;
  _neps = bunch._neps;
  _seps = bunch._seps;
  _feps = bunch._feps;
  _qamp = bunch._qamp;
  _amps = bunch._amps;
  _vols = bunch._vols;
  _de = bunch._de;

  return *this;
}

Bunch::~Bunch () {}

const std::vector<double>& Bunch::Samples(const std::string var) const {

  return _data[var];
}

const std::vector<double>& Bunch::Errors(const std::string var) const {

  return _data.Errors(var);
}

double Bunch::Radius(const size_t i) const {

  return sqrt(pow(_data["x"][i], 2) + pow(_data["y"][i], 2));
}

std::vector<double> Bunch::Radii() const {

  std::vector<double> radii;
  size_t i;
  for (i = 0; i < Size(); i++)
      radii.push_back(Radius(i));
  return radii;
}

Variable Bunch::Mean(const std::string& var) const {

  Variable mean;
  try {
    std::vector<double> samples, errors;
    Arrays(var, samples, errors);

//    mean.SetValue(Math::TMean(samples, .05));
//    mean.SetSError(Math::TMeanSError(samples, .05));
    mean.SetValue(Math::Mean(samples));
    mean.SetSError(Math::MeanSError(samples));
    if ( errors.size() )
        mean.SetMError(Math::MeanMError(errors));
    return mean;
  } catch ( Exceptions::Exception& e ) {
    throw(Exceptions::Exception(Exceptions::recoverable,
	  "Failed to compute mean of "+var+std::string(e.what()),
	  "Bunch::Mean"));
    return mean;
  }
}

Variable Bunch::Alpha(const std::string axis) const {

  if ( _dim == 2 )
      return _alpha;

  // Create a sub trace space covariance matrix with only the requested axis
  Variable alpha;
  Matrix<double> covmat(2, 2);
  std::vector<std::string> vars = {axis, "p"+axis};
  size_t i, j;
  for (i = 0; i < 2; i++)
    for (j = 0; j < 2; j++)
	covmat[i][j] = _data.T(vars[i], vars[j]);

  alpha.SetValue(-covmat[0][1]/sqrt(covmat.Determinant()));

  return alpha;
}

Variable Bunch::Beta(const std::string axis) const {

  if ( _dim == 2 )
      return _beta;

  // Create a sub covariance matrix with only the requested axis
  Variable beta;
  Matrix<double> covmat(2, 2);
  std::vector<std::string> vars = {axis, "p"+axis};
  size_t i, j;
  for (i = 0; i < 2; i++)
    for (j = 0; j < 2; j++)
	covmat[i][j] = _data.T(vars[i], vars[j]);

  beta.SetValue(covmat[0][0]/sqrt(covmat.Determinant()));

  return beta;
}

Variable Bunch::Gamma(const std::string axis) const {

  if ( _dim == 2 )
      return _gamma;

  // Create a sub covariance matrix with only the requested axis
  Variable gamma;
  Matrix<double> covmat(2, 2);
  std::vector<std::string> vars = {axis, "p"+axis};
  size_t i, j;
  for (i = 0; i < 2; i++)
    for (j = 0; j < 2; j++)
	covmat[i][j] = _data.T(vars[i], vars[j]);

  gamma.SetValue(covmat[1][1]/sqrt(covmat.Determinant()));

  return gamma;
}

Variable Bunch::NormEmittanceEstimate(const SumStat& stat) const {

  try {
    // Check that the summary statistic is fractional, throw otherwise
    if ( !SumStatDict[stat].frac )
        throw(Exceptions::Exception(Exceptions::recoverable,
	      "The requested summary statistic is not fractional: "+SumStatDict[stat].name,
	      "Bunch::NormEmittanceEstimate"));

    // Find the radius that would correspond to this volume for a Gaussian bunch
    DGaus fgaus(_dim);
    double R = fgaus.Radius(_fraction);

    // Return the corresponding estimate of the normalised emittance
    Variable neps;
    if ( stat == amp ) {
      neps.SetValue(_qamp.GetValue()/(R*R));
      neps.SetSError(neps.GetValue()*_qamp.GetSError()/_qamp.GetValue());

    } else if ( stat == subeps ) {
      neps.SetValue(_fraction*_seps.GetValue()/TMath::Gamma((double)_dim/2+1, R*R/2));
      neps.SetSError(neps.GetValue()*_seps.GetSError()/_seps.GetValue());

    } else if ( stat == vol ) {
      neps.SetValue(pow(std::tgamma((double)_dim/2+1)*_feps.GetValue(), 2./_dim)/_mass/M_PI/(R*R));
      neps.SetSError(neps.GetValue()*(2./_dim)*_seps.GetSError()/_seps.GetValue());
    }

    return neps;
  } catch ( Exceptions::Exception& e ) {
    throw(Exceptions::Exception(Exceptions::recoverable,
	  "Failed to compute"+std::string(e.what()),
	  "Bunch::NormEmittanceEstimate"));
  }
}

Variable Bunch::SummaryStatistic(const SumStat& stat) const {

  try {
    // Switch on the types of summary statistics
    switch(stat) {
      case alpha:
        return Alpha();
        break;
      case beta:
        return Beta();
        break;
      case gamma:
        return Gamma();
        break;
      case mecl:
        return MechanicalL();
        break;
      case eps:
        return Emittance();
        break;
      case neps:
        return NormEmittance();
        break;
      case amp:
	return AmplitudeQuantile();
	break;
      case subeps:
	return SubEmittance();
	break;
      case vol:
	return FracEmittance();
	break;
      case mom:
        return Mean("ptot");
        break;
      case trans:
        return Variable(1.);
        break;
    }

    return Variable(0.);
  } catch ( Exceptions::Exception& e ) {
    throw(Exceptions::Exception(Exceptions::recoverable,
	  "Could not produce the requested summary statistic"+std::string(e.what()),
	  "Bunch::SummaryStatistic"));
  }
}

void Bunch::SetCoreFraction(const double frac) {

  try {
    _fraction = frac;
    SetCoreSize(frac*Size());
  } catch ( Exceptions::Exception& e ) {
    throw(Exceptions::Exception(Exceptions::recoverable,
	  "Failed to set with fraction "+std::to_string(frac)+std::string(e.what()),
	  "Bunch::SetCoreFraction"));
  } 
}

void Bunch::SetCoreSize(const size_t size) {

  // Check that the number of samples requested is possible
  size_t N = Size();
  Assert::IsLower("Bunch::SetCoreSize", "The sample size", size, N, true);

  // Use the requested figure of merit to get the optimal subsample
  std::vector<std::pair<size_t, double>> foms(N);	// Array of figures-of-merit
  size_t i, j;

  // Store all the single particle amplitudes
  for (i = 0; i < N; i++) {
    foms[i].first = i;
    foms[i].second = _amps[i];
  }

  // Sort the figures-of-merit
  std::sort(foms.begin(), foms.end(),
	    [] (const std::pair<size_t, double>& a, const std::pair<size_t, double>& b) {
		  return a.second < b.second;
	    });

  // Fill the subsamples and subsample errors according to the selectred FOM
  BunchMap subsamples, suberrors;
  std::vector<std::string> keys = _data.Keys();
  keys.push_back("pz");
  for (const std::string& key : keys) {
    for (j = 0; j < size; j++)
      subsamples[key].push_back(_data[key][foms[j].first]);
    if ( _data.PhaseSpaceErrors() )
      for (j = 0; j < size; j++)
          suberrors[key].push_back(_data.Errors(key)[foms[j].first]);
  }

  // Set the core object
  _core = BunchData(subsamples, suberrors);

  // Set the alpha-amplitude
  SetAmplitudeQuantile(foms[size-1].second);

  // Set the alpha-subemittance
  SetSubEmittance();
}

void Bunch::SetCoreVolume(const std::string algo,
		       	  const double norm) {

  Assert::IsProbability("Bunch::Volume", "The sample fraction", _fraction, true, false);
  try {
    if ( algo == "hull" ) {
      // Create a new vector that contains all of the core nD points in a single array,
      // pass it to qhull, extract the volume of the hull
      std::vector<double> points;
      size_t i, j;
      for (i = 0; i < CoreSize(); i++)
        for (j = 0; j < _dim; j++)
    	    points.push_back(_core.PhaseSpace()[j][i]);

      orgQhull::Qhull qhull("", _dim, CoreSize(), &(points[0]), "");

      // Correct the measured for volume for the natural bias of a convex hull
      _feps.SetValue(qhull.volume()/Math::HullVolumeTGausRelativeBias(_dim, _fraction, Size()));

      // Record the statistical uncertainty on the convex hull
      _feps.SetSError(_feps.GetValue()*Math::HullVolumeTGausRelativeRMS(_dim, _fraction, Size()));

    } else if ( algo == "alpha" ) {
      // Create an alpha complex of the core points and return its volume
      Matrix<double> points = _core.PhaseSpace().Transpose();

      // Find the level of the contour
      // Figure out the errors? Tall order. (TODO)
      AlphaComplex alphac(points.std(), 0.01);
      _feps.SetValue(alphac.GetVolume());

    } else if ( algo == "de" ) {

      // Feed the samples to the density estimator
      if ( !_de.GetDimension() ) {
        Matrix<double> points = _data.PhaseSpace().Transpose();
        _de = DensityEstimator(points.std(), algo, false, norm);
      }

      // Compute the volume of the contour
      _feps.SetValue(_de.ContourVolume(_fraction, "mc", false));
      _feps.SetSError(_de.ContourVolumeError());

    } else {
      throw(Exceptions::Exception(Exceptions::recoverable,
	    "Phase-space volume computation algorithm not recognized: "+algo,
	    "Bunch::Volume"));
    }
  } catch ( Exceptions::Exception& e ) {
    throw(Exceptions::Exception(Exceptions::recoverable,
	  "Failed to compute with algorithm: "+algo+std::string(e.what()),
	  "Bunch::Volume"));
  }
}

void Bunch::SetCorrectedAmplitudes() {

  // Compute the inverse covariance matrix and the means in each projection
  size_t N = Size();
  size_t n = 100; // Number of times the order needs to be refreshed
  Matrix<double> samples = _data.PhaseSpace().Transpose();
  Matrix<double> covmat = _data.S();
  Matrix<double> invcovmat = _data.S().Inverse();
  std::vector<double> means(_dim);
  double neps = NormEmittanceValue(_data.S());
  size_t i, j;
  for (i = 0; i < _dim; i++)
      means[i] = Math::Mean(_data.PhaseSpace()[i]);

  // For each particle, compute the product neps*vec^T*invcovmat*vec
  // Sort the amplitudes in a descending order and retain the particle id they are associated with
  std::vector<std::pair<size_t, double>> amps(N);
  for (i = 0; i < N; i++)
      amps[i] = std::pair<size_t, double>(i, AmplitudeValue(invcovmat, means, samples[i], neps));

  std::sort(amps.begin(), amps.end(), [] (const std::pair<size_t, double>& a,
	const std::pair<size_t, double>& b) { return a.second < b.second; });

  // Start with the highest amplitude, remove one particle at each step
  // Fast to upload means and cov. matrix, slow to recompute amplitudes... (TODO?)
  _amps.resize(N);
  while ( amps.size() > _dim ) {

    // Add the current highest amplitude
    _amps[amps.back().first] = amps.back().second;

    // Update the inverse covariance matrix, the emittance and the means
    Math::DecrementCovarianceMatrix(amps.size(), covmat, means, samples[amps.back().first]);
    invcovmat = covmat.Inverse();
    neps = NormEmittanceValue(covmat);
    for (j = 0; j < _dim; j++)
	Math::DecrementMean(amps.size(), means[j], samples[amps.back().first][j]);

    // Remove the amplitude from the list, recompute the last n amplitudes in the sample
    amps.pop_back();

    // If requested, reorder the remaining amplitudes according to the new covariance matrix
    if ( amps.size()%(N/n) == 0 ) {
      for (i = 0; i < amps.size(); i++)
	  amps[i].second = AmplitudeValue(invcovmat, means, samples[amps[i].first], neps);

      // Sort the updated amplitudes
      std::sort(amps.begin(), amps.end(), [] (const std::pair<size_t, double>& a,
		const std::pair<size_t, double>& b) { return a.second < b.second; });
    }
  }

  // The last _dim amplitudes is necessarly 0, as they have singular covariances
  for (i = 0; i < _dim; i++)
      _amps[amps[amps.size()-1-i].first] = 0.;
}

void Bunch::SetMCDAmplitudes() {

  // Compute the inverse covariance matrix and the means in each projection
  Matrix<double> covmat = Math::RobustCovarianceMatrix(_data.PhaseSpace());
  Matrix<double> invcovmat = covmat.Inverse();
  std::vector<double> means(_dim);
  size_t i;
  for (i = 0; i < _dim; i++)
      means[i] = Math::RobustMean(_data.PhaseSpace()[i]);

  // For each particle, compute the product eps*vec^T*invcovmat*vec
  _amps.resize(Size());
  for (i = 0; i < _amps.size(); i++)
      _amps[i] = AmplitudeValue(invcovmat, means, _data.PhaseSpace().Column(i), _neps.GetValue());
}

void Bunch::SetGeneralisedAmplitudes(const double norm) {

  // Feed the samples to the density estimator
  if ( !_de.GetDimension() ) {
    Matrix<double> points = _data.PhaseSpace().Transpose();
    _de = DensityEstimator(points.std(), "knn");

    gStyle->SetOptStat(0);
    InfoBox info("Preliminary", "2.9.1", "1.2_10mm", "2016/04");
    TCanvas *c = new TCanvas("c", "c", 1200, 800);
    TH2F* graph = _de.Graph2D(-100, 100, -80, 80, _data.Id("x"), _data.Id("px"), {0, 0, 0, 0}); 

    // Find the contour level (compute the DE in all the points
    /*Matrix<double> samples = _data.PhaseSpace().Transpose();
    std::vector<double> densities(samples.Nrows());
    for (size_t i = 0; i < samples.Nrows(); i++) {
	densities[i] = _de(samples[i]);
    }
    std::sort(densities.begin(), densities.end());
    double level = densities[.91*densities.size()];
    std::cerr << level << std::endl;
    graph->SetMinimum(level);*/
    

    graph->Draw("CONTZ");
    graph->SetTitle(";x [mm];p_{x}  [MeV/c];#rho [mm^{-2}(MeV/c)^{-2}]");
    info.SetPosition("tl");
    info.Draw();
    c->SetRightMargin(0.15);
    c->SaveAs("de_knn.pdf");
    delete c;
  }

  // For each particle, compute the density
  _amps.resize(Size());
  size_t i;
  for (i = 0; i < _amps.size(); i++)
      _amps[i] = _de(_data.PhaseSpace().Column(i));

  // Compute the contour volume
/* double frac = tgamma((double)_dim/2.)*TMath::Gamma((double)_dim/2., .5);
  double volume = _de.ContourVolume(frac*norm, "mc", false);

  // Find the maximum of the probability distribution
  double max = *std::max_element(_amps.begin(), _amps.end());
//  double max = 1./(pow(2, (double)_dim/2)*tgamma((double)_dim/2+1)*volume);

  // Compute the estimated emittance (prefactor)
  double neps = NormEmittanceEstimate(frac, volume);*/

  // For each particle, compute the generalised distance
  for (i = 0; i < _amps.size(); i++)
//      _amps[i] = neps*Math::GeneralisedDistanceSquared(_amps[i], max);
      _amps[i] = -log(_amps[i]);
}

void Bunch::SetVoronoiVolumes() {
  
  try {
    Voronoi vor((_data.PhaseSpace()).std(), false);
    for (const Hull& cell : vor.GetCellArray()) {
      if ( cell.GetVolume() >= 0 ) {
        _vols.push_back(cell.GetVolume());
      } else {
        _vols.push_back(DBL_MAX);
      }
    }
  } catch ( Exceptions::Exception& e ) {
    throw(Exceptions::Exception(Exceptions::nonRecoverable,
	  "Could not set"+std::string(e.what()),
	  "Bunch::SetVoronoiVolumes"));
  }
}

TEllipse* Bunch::Ellipse(const std::string& vara,
			 const std::string& varb,
			 const double p) const {

  TEllipse *ell = NULL;
  try {
    AssertContains(vara);
    AssertContains(varb);

    std::vector<double> mean = {Math::Mean(_data[vara]), Math::Mean(_data[varb])};
    Matrix<double> covmat( { {_data.S(vara, vara), _data.S(vara, varb)},
			     {_data.S(varb, vara), _data.S(varb, varb)} } );

    DGaus gaus(mean, covmat);
    ell = (TEllipse*)gaus.Contour2D(p)[0];
    return ell;
  } catch ( Exceptions::Exception& e ) {
    throw(Exceptions::Exception(Exceptions::recoverable,
	  "Could not produce an ellipse"+std::string(e.what()),
	  "Bunch::Ellipse"));
    return ell;
  }
}

TEllipse* Bunch::RobustEllipse(const std::string& vara,
			       const std::string& varb,
			       const double p) const {

  TEllipse *ell = NULL;
  try {
    AssertContains(vara);
    AssertContains(varb);

    std::vector<double> mean = {Math::RobustMean(_data[vara]),
				Math::RobustMean(_data[varb])};
    Matrix<double> samples({_data[vara], _data[varb]});
    Matrix<double> covmat = Math::RobustCovarianceMatrix(samples);

    DGaus gaus(mean, covmat);
    ell = (TEllipse*)gaus.Contour2D(p)[0];
    return ell;
  } catch ( Exceptions::Exception& e ) {
    throw(Exceptions::Exception(Exceptions::recoverable,
	  "Could not produce an MCD ellipse"+std::string(e.what()),
	  "Bunch::RobustEllipse"));
    return ell;
  }
}

TH1F* Bunch::Histogram(const std::string& var,
		       double min,
		       double max) const {
  
  TH1F *hist = NULL;
  try {
    AssertContains(var);

    Drawer drawer;
    if ( max != min )
        drawer.SetVariableLimits(var, min, max);

    hist = drawer.New1DHistogram(_name, var);
    hist->FillN(Size(), &Samples(var)[0], NULL);
    return hist;
  } catch ( Exceptions::Exception& e ) {
    throw(Exceptions::Exception(Exceptions::recoverable,
	  "Could not produce a 1D histogram"+std::string(e.what()),
	  "Bunch::Histogram"));
    return hist;
  }
}

TH2F* Bunch::Histogram(const std::string& vara, 
		       const std::string& varb,
		       double mina,
		       double maxa,
		       double minb,
		       double maxb) const {

  TH2F *hist = NULL;
  try {
    AssertContains(vara);
    AssertContains(varb);

    Drawer drawer;
    if ( maxa != mina )
        drawer.SetVariableLimits(vara, mina, maxa);
    if ( maxb != minb )
        drawer.SetVariableLimits(varb, minb, maxb);
    hist = drawer.New2DHistogram(_name, vara, varb);
    hist->FillN(Size(), &Samples(vara)[0], &Samples(varb)[0], NULL, 1);
    return hist;
  } catch ( Exceptions::Exception& e ) {
    throw(Exceptions::Exception(Exceptions::recoverable,
	  "Could not produce a 2D histogram"+std::string(e.what()),
	  "Bunch::Histogram"));
    return hist;
  }
}

ScatterGraph* Bunch::AmplitudeScatter(const std::string& vara,
				      const std::string& varb) const {

  ScatterGraph *scat = NULL;
  try {
    AssertContains(vara);
    AssertContains(varb);

    Drawer drawer;
    scat = drawer.NewAmplitudeScatter(_name, vara, varb);
    std::vector<double> x(Samples(vara)), y(Samples(varb)), z(_amps);
    scat->SetPoints(Size(), &x[0], &y[0], &z[0]);
    scat->SetOrder(descending);
    scat->SetMaxSize(1e4);
    return scat;
  } catch ( Exceptions::Exception& e ) {
    throw(Exceptions::Exception(Exceptions::nonRecoverable,
	  "Could not produce a scatter plot"+std::string(e.what()),
	  "Bunch::AmplitudeScatter"));
    return scat;
  }
}

void Bunch::Initialize(const BunchMap& samples,
		       const BunchMap& errors) {

  // Initialize the underlying bunch data
  _data = BunchData(samples, errors);

  // Set the dimension
  _dim = samples.size()-1;

  // Set the geometric emittance
  SetEmittance();

  // Set the normalised emittance
  SetNormEmittance();

  // Compute the other bunch parameters from the geometric emittance
  SetTwiss();

  // Set all the individual particle amplitudes
  SetAmplitudes();
}

void Bunch::Arrays(const std::string& var,
	           std::vector<double>& samples,
	    	   std::vector<double>& errors) const {

  if ( Contains(var) ) {
    samples = _data[var];
    if ( _data.PhaseSpaceErrors() )
        errors = _data.Errors(var);
  } else if ( var == "ptot" ) {
    samples = TotalMomenta();
    if ( _data.PhaseSpaceErrors() )
        errors = TotalMomentumErrors();
  } else if ( var == "amp" ) {
    samples = Amplitudes();
  } else {
    throw(Exceptions::Exception(Exceptions::recoverable,
	  "Variable not recognized: "+var,
	  "Bunch::Arrays"));
  }
}

double Bunch::TotalMomentum(const size_t i) const {

  double ptot(0);
  for (const std::string& axis : _data.Axes())
      ptot += pow(_data["p"+axis][i], 2);
  ptot += pow(_data["pz"][i], 2);
  return sqrt(ptot);
}

std::vector<double> Bunch::TotalMomenta() const {

  std::vector<double> ptots;
  size_t i;
  for (i = 0; i < Size(); i++)
      ptots.push_back(TotalMomentum(i));
  return ptots;
}

double Bunch::TotalMomentumError(const size_t i) const {

  double ptot = TotalMomentum(i);
  if ( !ptot )
      return 0.;

  double ptoterr(0);
  for (const std::string& axis : _data.Axes())
      ptoterr += pow(_data["p"+axis][i]*_data.Errors("p"+axis)[i], 2);
  ptoterr += pow(_data["pz"][i]*_data.Errors("pz")[i], 2);
  return sqrt(ptoterr)/ptot;
}

std::vector<double> Bunch::TotalMomentumErrors() const {

  std::vector<double> ptoterrs;
  size_t i;
  for (i = 0; i < Size(); i++)
      ptoterrs.push_back(TotalMomentumError(i));
  return ptoterrs;
}

void Bunch::SetTwiss() {

  double Vxxp = _data.T("x", "px");
  double Vyyp = _data.T("y", "py");
  _alpha.SetValue(-(Vxxp+Vyyp)/(2*_eps.GetValue()));

  double Vxx = _data.T("x", "x");
  double Vyy = _data.T("y", "y");
  _beta.SetValue((Vxx+Vyy)/(2*_eps.GetValue()));

  double Vxpxp = _data.T("px", "px");
  double Vypyp = _data.T("py", "py");
  _gamma.SetValue((Vxpxp+Vypyp)/(2*_eps.GetValue()));

  double Vxpy = _data.T("x", "py");
  double Vypx = _data.T("y", "px");
  _mecl.SetValue(-(Vxpy-Vypx)/(2*_eps.GetValue()));

  // If the errors are provided, compute the measurement errors
  if ( _data.PhaseSpaceErrors() ) {
    // Fetch the means
    std::map<std::string, double> means;
    for (const std::string& key : _data.Keys())
        means[key] = Math::Mean(_data.Opticals(key));

    // Get the value of the determinant to the right power, comes in the computation
    double r = (1./_dim-1);
    double detr = pow(_data.T().Determinant(), r);

    // Get the cofactor matrix
    Matrix<double> C = _data.T().CofactorMatrix();

    // Compute the full measurement errors
    _alpha.SetMError(TwissMError("alpha", means, detr, C));
    _beta.SetMError(TwissMError("beta", means, detr, C));
    _gamma.SetMError(TwissMError("gamma", means, detr, C));
    _mecl.SetMError(0.); // TODO TODO TODO
  }

  // Transverse beta function statistical error
  _alpha.SetSError(TwissSError(_alpha.GetValue()));
  _beta.SetSError(TwissSError(_beta.GetValue()));
  _gamma.SetSError(TwissSError(_gamma.GetValue()));
  _mecl.SetSError(TwissSError(_mecl.GetValue()));
}

double Bunch::TwissMError(const std::string& name,
		     	 std::map<std::string, double>& means,
		     	 const double& detr,
		     	 const Matrix<double>& C) const {

  std::vector<std::vector<std::string>> prefixes;	// Prefixes for this twiss parameter
  double twiss;						// Value of the Twiss parameter
  if ( name == "alpha" ) {
    twiss = _alpha.GetValue();
    prefixes = {{"", "p"}, {"p", ""}};
  } else if ( name == "beta" ) {
    twiss = _beta.GetValue();
    prefixes = {{"", ""}};
  } else if ( name == "gamma" ) {
    twiss = _gamma.GetValue();
    prefixes = {{"p", "p"}};
  } else {
    throw(Exceptions::Exception(Exceptions::recoverable,
	  "Twiss parameter not recognized: "+name,
	  "Bunch::TwissMError"));
    return 0.;
  }

  // Compute the full error
  double dimfact =  pow(2., -2*((int)_data.Axes().size()-1));
  double term, sum(0.);
  size_t i;
  size_t N = Size();
  for (const std::string& axis : _data.Axes()) {
    for (i = 0; i < N; i++) {

      term = 0.;
      for (const std::vector<std::string>& p : prefixes) {

	// First addition term dimfact*Sigma_q(')q(')q(')q(')
	term +=   dimfact*pow((_data.Opticals(p[0]+axis)[i]-means[p[0]+axis])
		* _data.Errors(p[1]+axis)[i], 2)/prefixes.size();

	// Second addition term beta*Sum_a*C_{aq(')}(a-<a>)*(q(')-<q(')>)sig_q(')^2
        for (const std::string& key : _data.Keys())
	    term -=   detr * twiss * C[_data.Id(key)][_data.Id(p[0]+axis)]
		    * (_data.Opticals(key)[i]-means[key])
		    * (_data.Opticals(p[1]+axis)[i]-means[p[1]+axis])
		    * pow(_data.Errors(p[0]+axis)[i], 2)/_dim/prefixes.size();

        // Increment the first term of the error
	sum += term;
      }
    }
  }

  // Return the combined error of the computed sum and the determinant error
  return sqrt(4*sum/pow((N-1)*_eps.GetValue(), 2) + pow(twiss*_eps.GetMError()/_eps.GetValue(), 2));
}

double Bunch::TwissSError(const double& twiss) const {

  return twiss*(_eps.GetSError()/_eps.GetValue());
}

void Bunch::SetEmittance() {

  // Set the geometric emittance
  double det = _data.T().Determinant();
  _eps.SetValue(pow(det, 1./_dim));

  // If the errors are provided, compute the measurement error
  if ( _data.TraceSpaceErrors() ) {
    double det_error = Math::DeterminantMError(_data.TraceSpace(), _data.TraceSpaceErrors());
    _eps.SetMError(pow(det, 1./_dim-1)*det_error/_dim);
  }

  // Emittance statistical error
  double det_error = Math::DeterminantSError(_data.TraceSpace());
  _eps.SetSError(pow(det, 1./_dim-1)*det_error/_dim);
}

void Bunch::SetNormEmittance() {

  // Set the normalised emittance
  double det = _data.S().Determinant();
  _neps.SetValue(pow(det, 1./_dim)/_mass);

  // If the errors are provided, compute the measurement error
  if ( _data.PhaseSpaceErrors() ) {
    double det_error = Math::DeterminantMError(_data.PhaseSpace(), _data.PhaseSpaceErrors());
    _neps.SetMError(pow(det, 1./_dim-1)*det_error/_dim/_mass);
  }

  // Emittance statistical error
  double det_error = Math::DeterminantSError(_data.PhaseSpace());
  _neps.SetSError(pow(det, 1./_dim-1)*det_error/_dim/_mass);
}

void Bunch::SetSubEmittance() {

  // Set the normalised emittance
  double det = _core.S().Determinant();
  _seps.SetValue(pow(det, 1./_dim)/_mass);

  // If the errors are provided, compute the measurement error
  if ( _core.PhaseSpaceErrors() ) {
    double det_error = Math::DeterminantMError(_core.PhaseSpace(), _core.PhaseSpaceErrors());
    _seps.SetMError(pow(det, 1./_dim-1)*det_error/_dim/_mass);
  }

  // Emittance statistical error
  double det_error = Math::DeterminantSError(_core.PhaseSpace());
  _seps.SetSError(pow(det, 1./_dim-1)*det_error/_dim/_mass);
}

void Bunch::SetAmplitudeQuantile(const double& qamp) {

  // Set the value
  _qamp.SetValue(qamp);

  // Set the statistical uncertainty based on the Gaussian core assumption
  DChiSquared fchi2(_dim);
  double eps = NormEmittanceEstimate(amp).GetValue();
  double density = fchi2(qamp/eps)/eps;
  _qamp.SetSError(Math::QuantileSError(Size(), _fraction, density));
}

double Bunch::NormEmittanceValue(const Matrix<double>& covmat) const {

  return pow(covmat.Determinant(), 1./_dim)/_mass;
}

void Bunch::SetAmplitudes() {

  // Compute the inverse covariance matrix and the means in each projection
  Matrix<double> invcovmat = _data.S().Inverse();
  std::vector<double> means(_dim);
  size_t i;
  for (i = 0; i < _dim; i++)
      means[i] = Math::Mean(_data.PhaseSpace()[i]);

  // For each particle, compute the product eps*vec^T*invcovmat*vec
  _amps.resize(Size());
  for (i = 0; i < _amps.size(); i++)
      _amps[i] = AmplitudeValue(invcovmat, means, _data.PhaseSpace().Column(i), _neps.GetValue());
}

double Bunch::AmplitudeValue(const Matrix<double>& invcovmat,
			    const std::vector<double>& means,
			    const std::vector<double>& vec,
			    const double& neps) const {

  double amp(0.), temp;
  size_t i, j;
  for (i = 0; i < _dim; i++) {
    temp = 0.;
    for (j = 0; j < _dim; j++)
        temp += invcovmat[i][j]*(vec[j]-means[j]);
    amp += (vec[i]-means[i])*temp;
  }

  return neps*amp;
}
} // namespace Beam
