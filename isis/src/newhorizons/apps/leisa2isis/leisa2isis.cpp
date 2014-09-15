#include "Isis.h"

#include "Cube.h"
#include "IException.h"
#include "IString.h"
#include "OriginalLabel.h"
#include "ProcessBySample.h"
#include "ProcessImportFits.h"
#include "Pvl.h"
#include "PvlGroup.h"
#include "PvlKeyword.h"
#include "UserInterface.h"
#include "iTime.h"

#include <fstream>
#include <iostream>
#include <QString>

using namespace std;
using namespace Isis;

void IsisMain() {

  UserInterface &ui = Application::GetUserInterface();

  ProcessImportFits importFits;
  importFits.setFitsFile(FileName(ui.GetFileName("FROM")));
  PvlGroup mainLabel;
  mainLabel = importFits.fitsLabel(0);

  // Get the first label and make sure this is a New Horizons LEISA file
  if (!mainLabel.hasKeyword("MISSION") || !mainLabel.hasKeyword("INSTRU") ||
      mainLabel["MISSION"][0] != "New Horizons" || mainLabel["INSTRU"][0] != "lei") {
    QString msg = QObject::tr("Input file [%1] does not appear to be a New Horizons LEISA FITS "
                              "file. Input file label value for MISSION is [%2], "
                              "INSTRU is [%3]").
                  arg(ui.GetFileName("FROM")).arg(mainLabel["MISSION"][0]).
                  arg(mainLabel["INSTRU"][0]);
    throw IException(IException::User, msg, _FILEINFO_);
  }

  // Check to see if the error image was requested from the FITS file and 
  // that it has the corresponding extension
  if (ui.WasEntered("ERRORMAP")) {
    PvlGroup extensionLabel = importFits.fitsLabel(5); 
    try {
      extensionLabel = importFits.fitsLabel(5); 
    }
    catch (IException &e) {
      QString msg = QObject::tr("Input file [%1] does not appear to be a calibrated New Horizons "
                                "LEISA FITS file. There is no errormap "
                                "extension").arg(ui.GetFileName("FROM"));

      throw IException(e, IException::Unknown, msg, _FILEINFO_);
    }

    if (!extensionLabel.hasKeyword("EXTNAME")) {
      QString msg = QObject::tr("Input file [%1] does not appear to contain a New Horizons LEISA "
                                "calibrated image. FITS label keyword EXTNAME is missing").
                             arg(ui.GetFileName("FROM"));
      throw IException(IException::User, msg, _FILEINFO_);
    }
    else if (extensionLabel["EXTNAME"][0] != "ERRORMAP") {
      QString msg = QObject::tr("Input file [%1] does not appear to contain a New Horizons LEISA "
                                "calibrated image. FITS label value for EXTNAME is [%2]").
                             arg(ui.GetFileName("FROM")).arg(extensionLabel["EXTNAME"][0]);
      throw IException(IException::User, msg, _FILEINFO_);
    }
  }

  // Check to see if the quality image was requested from the FITS file and 
  // that it has the corresponding extension
  if (ui.WasEntered("QUALITY")) {
    PvlGroup  extensionLabel = importFits.fitsLabel(6); 
    try {
      extensionLabel = importFits.fitsLabel(6); 
    }
    catch (IException &e) {
      QString msg = QObject::tr("Input file [%1] does not appear to be a calibrated New Horizons "
                                "LEISA FITS file. There is no quality "
                                "extension").arg(ui.GetFileName("FROM"));

      throw IException(e, IException::Unknown, msg, _FILEINFO_);
    }

    if (!extensionLabel.hasKeyword("EXTNAME")) {
      QString msg = QObject::tr("Input file [%1] does not appear to contain a New Horizons LEISA "
                                "calibrated image. FITS label keyword EXTNAME is missing").
                             arg(ui.GetFileName("FROM"));
      throw IException(IException::User, msg, _FILEINFO_);
    }
    else if (extensionLabel["EXTNAME"][0] != "QUALITY") {
      QString msg = QObject::tr("Input file [%1] does not appear to contain a New Horizons LEISA "
                                "calibrated image. FITS label value for EXTNAME is [%2]").
                             arg(ui.GetFileName("FROM")).arg(extensionLabel["EXTNAME"][0]);
      throw IException(IException::User, msg, _FILEINFO_);
    }
  }






  // Import the primary image (LEISA raw/calibrated)
  importFits.SetOrganization(ProcessImport::BIL);
  importFits.setProcessFileStructure(0);

  Cube *output = importFits.SetOutputCube("TO");

  // Get the directory where the New Horizons translation tables are.
  PvlGroup dataDir(Preference::Preferences().findGroup("DataDirectory"));
  QString transDir = (QString) dataDir["NewHorizons"] + "/translations/";

  // Temp storage of translated labels
  Pvl outLabel;

  // Get the FITS label
  Pvl fitsLabel;
  fitsLabel.addGroup(importFits.fitsLabel(0));

  // Create an Instrument group
  FileName insTransFile(transDir + "leisaInstrument_fit.trn");
  PvlTranslationManager insXlater(fitsLabel, insTransFile.expanded());
  insXlater.Auto(outLabel);

  // Modify/add Instument group keywords not handled by the translater
//  PvlGroup &inst = outLabel.findGroup("Instrument", Pvl::Traverse);
//  QString target = (QString)inst["TargetName"];
//  if (target.startsWith("RADEC=")) {
//    inst.addKeyword(PvlKeyword("TargetName", "Sky"), PvlGroup::Replace);
//  }

  output->putGroup(outLabel.findGroup("Instrument", Pvl::Traverse));
/*
  // Create a Band Bin group
  FileName bandTransFile(transDir + "leisaBandBin_fit.trn");
  PvlTranslationManager bandBinXlater(fitsLabel, bandTransFile.expanded());
  bandBinXlater.Auto(outLabel);
  output->putGroup(outLabel.findGroup("BandBin", Pvl::Traverse));

  // Create an Archive group
  FileName archiveTransFile(transDir + "leisaArchive_fit.trn");
  PvlTranslationManager archiveXlater(fitsLabel, archiveTransFile.expanded());
  archiveXlater.Auto(outLabel);
  output->putGroup(outLabel.findGroup("Archive", Pvl::Traverse));
*/
  // Create a Kernels group
  FileName kernelsTransFile(transDir + "leisaKernels_fit.trn");
//  FileName kernelsTransFile("./leisaKernels_fit.trn");
  PvlTranslationManager kernelsXlater(fitsLabel, kernelsTransFile.expanded());
  kernelsXlater.Auto(outLabel);
  output->putGroup(outLabel.findGroup("Kernels", Pvl::Traverse));


  // Modify/add Instument group keywords not handled by the translater
  Pvl *isisLabel = output->label();
  PvlGroup &inst = isisLabel->findGroup("Instrument", Pvl::Traverse);

  //Add StartTime & EndTime
  QString midTime = inst["MidObservationTime"];
  iTime midTimeBetter(midTime.toDouble());

  QString obsDuration = inst["ObservationDuration"];
  double obsSeconds = obsDuration.toDouble();

  //do stuff with Isis time class to calculate startTime
  iTime startTime = midTimeBetter - obsSeconds/2.0;
  iTime endTime = midTimeBetter + obsSeconds/2.0;
  inst.addKeyword(PvlKeyword("StartTime", startTime.UTC()), PvlGroup::Replace);
  inst.addKeyword(PvlKeyword("EndTime", endTime.UTC()), PvlGroup::Replace);

  //Add FrameRate
  QString exposureTime = inst["ExposureDuration"];
  double frameRate = 1.0/exposureTime.toDouble(); //in Hz!
  inst.addKeyword(PvlKeyword("FrameRate", QString::number(frameRate)), PvlGroup::Replace); 
  inst.findKeyword("FrameRate").setUnits("Hz");
  // Save the input FITS label in the Cube original labels
  Pvl pvl;
  pvl += importFits.fitsLabel(0);
  OriginalLabel originals(pvl);
  output->write(originals);

  // Convert the main image data
  importFits.Progress()->SetText("Importing main LEISA image");
  importFits.StartProcess();
  importFits.ClearCubes();
  importFits.Finalize();


  // Import the ERRORMAP image. It is the 6th image in the FITS file (i.e., 5th extension)
  if (ui.WasEntered("ERRORMAP")) {
    PvlGroup extensionLabel = importFits.fitsLabel(5);
    importFits.SetOrganization(ProcessImport::BSQ);
    importFits.setProcessFileStructure(5);
    Cube *output = importFits.SetOutputCube("ERRORMAP");

    // Save the input FITS label in the Cube original labels
    Pvl origLabel;
    origLabel += extensionLabel;
    OriginalLabel originals(origLabel);
    output->write(originals);

    // Convert the main image data
    importFits.Progress()->SetText("Importing LEISA errormap image");
    importFits.StartProcess();
    importFits.ClearCubes();
  }

  // Import the quality image. It is the 7th image in the FITS file (i.e., 6th extension)
  if (ui.WasEntered("QUALITY")) {
    PvlGroup extensionLabel = importFits.fitsLabel(6);
    importFits.SetOrganization(ProcessImport::BSQ);
    importFits.setProcessFileStructure(6);
    Cube *output = importFits.SetOutputCube("QUALITY");

    // Save the input FITS label in the Cube original labels
    Pvl origLabel;
    origLabel += extensionLabel;
    OriginalLabel originals(origLabel);
    output->write(originals);

    // Convert the main image data
    importFits.Progress()->SetText("Importing LEISA quality image");
    importFits.StartProcess();
    importFits.ClearCubes();
  }



}



