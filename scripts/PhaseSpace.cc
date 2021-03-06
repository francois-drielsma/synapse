// Cpp includes
#include <iostream>
#include <vector>
#include <map>

// Additional modules
#include "Globals.hh"
#include "Extractor.hh"

/** @file  PhaseSpace.cc
 *
 *  @brief Main phase space density evolution code.
 *
 *	   Reconstructs a broad set of phase space statistics and outputs their evolution
 *	   along the beam line to ROOT TCanvases and ROOT TFiles.
 **/

/** @brief	Main function
 *
 *  @param	argc		Number of command line arguments
 *  @param	argv		List of command line aguments
 */
int main(int argc, char ** argv) {

  // Reroot the info messages to the terminal for now (TODO)
  Pitch::setAnOutput(Pitch::info, std::cerr);

  // Emittance algorithm global parameters (stored in globals)
  Globals &globals = Globals::GetInstance(argc, argv);

  // Types and plane ids of the data to extract. Empty means import all (recmc and data)
  std::vector<std::string> types;
  for (const std::string type : {"utruth", "truth", "recmc", "data"})
    if ( globals[type] )
	types.push_back(type);

  size_t min_vid((size_t)globals["min_vid"]), max_vid((size_t)globals["max_vid"]);
  std::map<std::string, std::vector<size_t>> plane_ids;
  for (const std::string type : {"utruth", "truth"})
    if ( globals[type] )
      for (size_t i = min_vid; i <= max_vid; i++)
          plane_ids[type].push_back(i);

  // Get the requested streams from the beam extractor
  Beam::Extractor ext(globals.GetDataFiles());
  std::string run_name = ext.GetRunName();
  std::map<std::string, Beam::Stream> streams = ext.GetStreams(types, plane_ids, globals["maxn"]);

  // If the corrected amplitudes are requested, set them
  // Set the core fraction of the streams, reconstruct the core volume
  for (const std::string& type : types) {

    // If density estimator is requested, set the levels, set the volumes, nothing else
    if ( globals["de"] ) {
      // Set the levels
      for (const size_t& i : streams[type].GetPlaneIds())
          streams[type][i].SetDensityLevels(streams[type].Transmission(i));

      // Set the core fraction of the beam to de
      streams[type].SetCoreFraction(globals["frac"], "de");

      // Set the volumes
      std::cerr << "setting volumes: " << streams[type].GetPlaneIds().size() << std::endl;
      for (const size_t& i : streams[type].GetPlaneIds()) {
	  std::cerr << i << std::endl;
          streams[type][i].SetCoreVolume("de", streams[type].Transmission(i));
	  std::cerr << streams[type][i].FracEmittance() << std::endl;
      }

     // Proceed
      continue;
    }

    // If corrected amplitudes are requested, set them
    if ( globals["corrected"] )
      for (const size_t& i : streams[type].GetPlaneIds())
          streams[type][i].SetCorrectedAmplitudes();

    // Set the core fraction of the beam to amplitudes
    streams[type].SetCoreFraction(globals["frac"]);    

    // Set the hull volumes
    for (const size_t& i : streams[type].GetPlaneIds())
        streams[type][i].SetCoreVolume();
  }

  // Intialize the drawer and the info box
  Beam::Drawer drawer;
  if ( globals["mice"] ) {
    std::string data_type = globals["data"] ? "Preliminary" : "[simulation]";
    std::string maus_version = ext.Get("MausVersion");
    InfoBox* info = new InfoBox(data_type, maus_version,
  	globals["run_name"].AsString(), globals["user_cycle"].AsString());
    drawer.SetInfoBox(info);
  }

  // List of requested evolution beam summary statistics
  std::vector<Beam::SumStat> stats;
  if ( !globals["de"] ) {
    stats = {Beam::alpha, Beam::beta, Beam::gamma, Beam::mecl, Beam::neps,
	 	Beam::amp, Beam::subeps, Beam::vol, Beam::mom, Beam::trans};
  } else {
    stats = {Beam::den, Beam::vol};
  }

  // Draw the evolution graphs for a set of selected summary statistics
  TFile *outfile = globals["de"] ?
	new TFile(TString::Format("%s_de_plots.root", ext.GetRunName().c_str()), "RECREATE") :
	new TFile(TString::Format("%s_plots.root", ext.GetRunName().c_str()), "RECREATE");	
  std::map<std::string, TGraphErrors*> graphs;
  for (const Beam::SumStat& stat : stats) {
    for (const std::string& type : types) {
      graphs[type] = streams[type].EvolutionGraph(stat);
      drawer.SetStyle(graphs[type], type);
      graphs[type]->SetName(TString::Format("%s_%s", graphs[type]->GetName(), type.c_str()));
      graphs[type]->Write();
    }

    if ( !Beam::SumStatDict[stat].frac ) {
      drawer.SaveMultiGraph(graphs, Beam::SumStatDict[stat].name);
    } else {
      drawer.SaveMultiGraph(graphs, Beam::SumStatDict[stat].name,
				streams[types[0]].FractionalFunction(stat));
    }
  }
  outfile->Close();
  
  // Move all the output to an appropriate directory
  Pitch::print(Pitch::info, "Moving the phase-space evolution graphs to "+ext.GetRunName());
  std::string sysCmd = "mkdir -p "+ext.GetRunName()+"; for name in";
  for (const Beam::SumStat& stat : stats)
      sysCmd += " "+Beam::SumStatDict[stat].name;
  sysCmd += "; do mv ${name}.pdf "+ext.GetRunName()+"; done; mv *_plots.root "+ext.GetRunName();
  if ( std::system(sysCmd.c_str()) )
      Pitch::print(Pitch::error, "Couldn't move plots");

  // If the density estimator is requested, ignore the fractional quantities
  if ( globals["de"] )
      return 0;

  // Draw the fractional graphs for a set of selected summary statistics
  outfile = new TFile(TString::Format("%s_frac.root", ext.GetRunName().c_str()), "RECREATE");
  std::vector<Beam::SumStat> fracstats = {Beam::amp, Beam::subeps, Beam::vol};
  std::map<Beam::SumStat, TGraphErrors*> buff;
  std::map<Beam::SumStat, std::map<std::string, TGraphErrors*>> fgraphs;
  for (const std::string& type : types) {
    buff = (type.find("truth") != std::string::npos) ? 
		streams[type].FractionalGraphs(globals["tku_vid"], globals["tkd_vid"]) :
		streams[type].FractionalGraphs(0, 5);
    for (const Beam::SumStat& stat : fracstats) {
      fgraphs[stat][type] = buff[stat];
      buff[stat]->SetName(TString::Format("%s_%s", buff[stat]->GetName(), type.c_str()));
      buff[stat]->Write();
    }
  }
  outfile->Close();

  for (const Beam::SumStat& stat : fracstats)
      drawer.SaveMultiGraph(fgraphs[stat], Beam::SumStatDict[stat].name+"_frac");

  // Move all the output to an appropriate directory
  Pitch::print(Pitch::info, "Moving the fractional graphs to "+ext.GetRunName()+"/frac");
  sysCmd = "mkdir -p "+ext.GetRunName()+"/frac; for name in";
  for (const Beam::SumStat& stat : fracstats)
      sysCmd += " "+Beam::SumStatDict[stat].name;
  sysCmd += "; do mv ${name}_frac.pdf "+ext.GetRunName()+
	"/frac; done; mv *_frac.root "+ext.GetRunName()+"/frac";
  if ( std::system(sysCmd.c_str()) )
      Pitch::print(Pitch::error, "Couldn't move fractional plots");
}
