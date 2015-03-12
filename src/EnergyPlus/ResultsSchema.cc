// C++ Headers
#include <cmath>
#include <string>
#include <iostream>
#include <fstream>
#include <random>

// ObjexxFCL Headers
#include <ObjexxFCL/environment.hh>
#include <ObjexxFCL/FArray.functions.hh>
#include <ObjexxFCL/Reference.fwd.hh>
#include <ObjexxFCL/FArray1D.hh>
#include <ObjexxFCL/Fmath.hh>
#include <ObjexxFCL/gio.hh>
#include <ObjexxFCL/string.functions.hh>

// EnergyPlus Headers
#include <ResultsSchema.hh>
#include <DataGlobalConstants.hh>
#include <DataHVACGlobals.hh>
#include <DataIPShortCuts.hh>
#include <DataPrecisionGlobals.hh>
#include <DisplayRoutines.hh>
#include <General.hh>
#include <GlobalNames.hh>
#include <InputProcessor.hh>
#include <UtilityRoutines.hh>

namespace EnergyPlus {

	namespace ResultsFramework {

		using namespace DataHVACGlobals;
		using namespace DataPrecisionGlobals;
		using namespace OutputProcessor;
		using DataGlobals::InitConvTemp;
		using DataGlobals::SecInHour;
		using DataGlobals::DisplayExtraWarnings;
		using General::TrimSigDigits;
		using General::RoundSigDigits;
		using OutputProcessor::RealVariables;
		using OutputProcessor::RealVariableType;

		static gio::Fmt fmtLD("*");

		std::unique_ptr<ResultsSchema> OutputSchema(new ResultsSchema);

		// getUUID and helper functions copied from shannon's code
		int randomInRange(int min, int max) {
			double scaled = (double)rand() / RAND_MAX;
			return (int)(max - min + 1)*scaled + min;
		}
		//return a version 4 (random) uuid
		std::string getNewUuid()
		{
			std::stringstream uuid_str;
			int four_low = 4096;
			int four_high = 65535;
			int three_low = 256;
			int three_high = 4095;
			uuid_str << std::hex << randomInRange(four_low, four_high);
			uuid_str << std::hex << randomInRange(four_low, four_high);
			uuid_str << "-" << std::hex << randomInRange(four_low, four_high);
			uuid_str << "-" << std::hex << randomInRange(four_low, four_high);
			uuid_str << "-4" << std::hex << randomInRange(three_low, three_high);
			uuid_str << "-8" << std::hex << randomInRange(three_low, three_high);
			uuid_str << std::hex << randomInRange(four_low, four_high);
			uuid_str << std::hex << randomInRange(four_low, four_high);
			return uuid_str.str();
		}



		// Class BaseResultObject
		BaseResultObject::BaseResultObject() {
			uuid = getNewUuid();
		}

		void BaseResultObject::setUUID(const std::string uuid_) {
			uuid = uuid_;
		}

		const std::string BaseResultObject::UUID() {
			return uuid;
		}
			


		// Class SimInfo
		void SimInfo::setProgramVersion(const std::string programVersion) {
			ProgramVersion = programVersion;
		}

		void SimInfo::setSimulationEnvironment(const std::string simulationEnvironment) {
			SimulationEnvironment = simulationEnvironment;
		}

		void SimInfo::setInputModelURI(const std::string inputModelURI) {
			InputModelURI = inputModelURI;
		}

		void SimInfo::setStartDateTimeStamp(const std::string startDateTimeStamp) {
			StartDateTimeStamp = startDateTimeStamp;
		}

		void SimInfo::setRunTime(const std::string elapsedTime) {
			RunTime = elapsedTime;
		}
		
		void SimInfo::setNumErrorsWarmup(const std::string numWarningsDuringWarmup, const std::string numSevereDuringWarmup) {
			NumWarningsDuringWarmup = numWarningsDuringWarmup;
			NumSevereDuringWarmup = numSevereDuringWarmup;
		}

		void SimInfo::setNumErrorsSizing(const std::string numWarningsDuringSizing, const std::string numSevereDuringSizing) {
			NumWarningsDuringSizing = numWarningsDuringSizing;
			NumSevereDuringSizing = numSevereDuringSizing;
		}

		void SimInfo::setNumErrorsSummary(const std::string numWarnings, const std::string numSevere) {
			NumWarnings = numWarnings;
			NumSevere = numSevere;
		}

		cJSON* SimInfo::GetJSON()
		{
			cJSON *_root, *_errsummary, *_errwarmup, *_errsizing;

			_root = cJSON_CreateObject();
			cJSON_AddItemToObject(_root, "UUID", cJSON_CreateString(uuid.c_str()));
			cJSON_AddItemToObject(_root, "ProgramVersion", cJSON_CreateString(ProgramVersion.c_str()));
			cJSON_AddItemToObject(_root, "SimulationEnvironment", cJSON_CreateString(SimulationEnvironment.c_str()));
			cJSON_AddItemToObject(_root, "InputModelURI", cJSON_CreateString(InputModelURI.c_str()));
			cJSON_AddItemToObject(_root, "StartDateTimeStamp", cJSON_CreateString(StartDateTimeStamp.c_str()));
			cJSON_AddItemToObject(_root, "RunTime", cJSON_CreateString(RunTime.c_str()));

			cJSON_AddItemToObject(_root, "ErrorSummary", _errsummary = cJSON_CreateObject());
			cJSON_AddItemToObject(_errsummary, "NumWarnings", cJSON_CreateString(NumWarnings.c_str()));
			cJSON_AddItemToObject(_errsummary, "NumSevere", cJSON_CreateString(NumSevere.c_str()));

			cJSON_AddItemToObject(_root, "ErrorSummaryWarmup", _errwarmup = cJSON_CreateObject());
			cJSON_AddItemToObject(_errwarmup, "NumWarnings", cJSON_CreateString(NumWarningsDuringWarmup.c_str()));
			cJSON_AddItemToObject(_errwarmup, "NumSevere", cJSON_CreateString(NumSevereDuringWarmup.c_str()));

			cJSON_AddItemToObject(_root, "ErrorSummarySizing", _errsizing = cJSON_CreateObject());
			cJSON_AddItemToObject(_errsizing, "NumWarnings", cJSON_CreateString(NumWarningsDuringSizing.c_str()));
			cJSON_AddItemToObject(_errsizing, "NumSevere", cJSON_CreateString(NumSevereDuringSizing.c_str()));

			return _root;
		}



		// Class Variable
		Variable::Variable(const std::string VarName, const int ReportFrequency, const int IndexType, const int ReportID, const std::string units) {
			varName = VarName;
			setReportFrequency(ReportFrequency);
			idxType = IndexType;
			rptID = ReportID;
			Units = units;
		}

		std::string Variable::variableName() {
			return varName;
		}

		void Variable::setVariableName(const std::string VarName) {
			varName = VarName;
		}
		
		std::string Variable::sReportFrequency() {
			return sReportFreq;
		}

		int Variable::iReportFrequency() {
			return iReportFreq;
		}
		
		void Variable::setReportFrequency(const int ReportFrequency) {
			iReportFreq = ReportFrequency;
			switch (iReportFreq)
			{
			case -1:  // each time UpdatedataandReport is called
				if (idxType == ZoneVar)
					sReportFreq = "Detailed - Zone";
				if (idxType == HVACVar)
					sReportFreq = "Detailed - HVAC";
				break;
			case 0:  // at 'EndTimeStepFlag'
				sReportFreq = "Timestep";
				break;
			case 1:  // at 'EndHourFlag'
				sReportFreq = "Hourly";
				break;
			case 2: // at 'EndDayFlag'
				sReportFreq = "Daily";
				break;
			case 3:  // at end of month
				sReportFreq = "Monthly";
				break;
			case 4:  // once per environment 'EndEnvrnFlag'
				sReportFreq = "RunPeriod";
				break;
			}
		}
		
		int Variable::indexType() {
			return idxType;
		}

		void Variable::setIndexType(int IndexType) {
			idxType = IndexType;
		}

		int Variable::reportID() {
			return rptID;
		}

		void Variable::setReportID(int Id) {
			rptID = Id;
		}

		std::string Variable::units() {
			return Units;
		}

		void Variable::setUnits(std::string units) {
			Units = units;
		}

		void Variable::pushValue(const double val) {
			Values.push_back(val);
		}

		std::vector<double>& Variable::values() {
			return Values;
		}

		cJSON* Variable::GetJSON() {
			cJSON *_root, *_errsummary, *_errwarmup, *_errsizing;

			_root = cJSON_CreateObject();
			cJSON_AddItemToObject(_root, "UUID", cJSON_CreateString(uuid.c_str()));
			cJSON_AddItemToObject(_root, "Name", cJSON_CreateString(varName.c_str()));
			cJSON_AddItemToObject(_root, "Units", cJSON_CreateString(Units.c_str()));
			cJSON_AddItemToObject(_root, "Frequency", cJSON_CreateString(sReportFreq.c_str()));

			return _root;
		}


		// class DataFrame
		DataFrame::DataFrame(std::string ReportFreq) {
			ReportFrequency = ReportFreq;
			RDataFrameEnabled = false;
			IDataFrameEnabled = false;
		}

		DataFrame::~DataFrame() {
		}

		void DataFrame::addVariable(Variable *var) {
			outputVariables.push_back(var);
			variableMap.insert(VarPtrPair(lastVariable()->reportID(), lastVariable()));
		}

		Variable* DataFrame::lastVariable() {
			return outputVariables.back();
		}

		void DataFrame::newRow(const int month, const int dayOfMonth, const int hourOfDay, const int curMin) {
			std::string ts = std::to_string(month) + "/" + std::to_string(dayOfMonth) + " " + std::to_string(hourOfDay) + ":" + std::to_string(curMin) + ":00";
			TS.push_back(ts);
		}

		bool DataFrame::rDataFrameEnabled() {
			return RDataFrameEnabled;
		}

		bool DataFrame::iDataFrameEnabled() {
			return IDataFrameEnabled;
		}

		void DataFrame::setRDataFrameEnabled(bool state) {
			RDataFrameEnabled = state;
		}

		void DataFrame::setIDataFrameEnabled(bool state) {
			IDataFrameEnabled = state;
		}

		void DataFrame::pushVariableValue(const int reportID, double value) {
			// this is O(1) complexity. I like this.
			variableMap[reportID]->pushValue(value); 
		}

		std::vector< Variable*> DataFrame::variables() {
			return outputVariables;
		}

		void DataFrame::writeFile() {
			std::string jsonfilename = "eplusout_" + ReportFrequency + ".json";
			std::ofstream jsonfile(jsonfilename);

			cJSON *_root, *_col, *_colfld, *_val, *_row, *_rowvec;

			_root = cJSON_CreateObject();
			cJSON_AddItemToObject(_root, "UUID", cJSON_CreateString(uuid.c_str()));
			cJSON_AddItemToObject(_root, "ReportFrequency", cJSON_CreateString(ReportFrequency.c_str()));
			cJSON_AddItemToObject(_root, "Cols", _col = cJSON_CreateArray());
			cJSON_AddItemToObject(_root, "Rows", _row = cJSON_CreateArray());

			if (jsonfile.is_open())
			{
				for (int i = 0; i < outputVariables.size(); ++i) {
					cJSON_AddItemToArray(_col, _colfld = cJSON_CreateObject());
					cJSON_AddStringToObject(_colfld, "Variable", outputVariables[i]->variableName().c_str()  );
					cJSON_AddStringToObject(_colfld, "UUID", outputVariables[i]->UUID().c_str() );
				}

				std::vector <double> vals;
				vals.reserve(10000);

				assert(TS.size() == outputVariables[0]->values().size());

				for (int row = 0; row < TS.size(); ++row) {
					vals.clear();
					for (int vars = 0; vars < outputVariables.size(); ++vars) {
						vals.push_back(outputVariables[vars]->values()[row]);
					}
					cJSON_AddItemToArray(_row, _rowvec = cJSON_CreateObject());
					cJSON_AddItemToObject(_rowvec, TS.at(row).c_str(), cJSON_CreateDoubleArray(&vals[0], vals.size()));
				}
			
				jsonfile << cJSON_Print(_root);
				jsonfile.close();
			}
			// does this need to go to error?
			else
				ShowWarningError("Unable to open file for time-series output.");
			
			cJSON_Delete(_root);
		}


		// Class ResultsSchema
		// initialize data frames
		DataFrame ResultsSchema::RIDetailedZoneTSData("Detailed-Zone"),
			ResultsSchema::RIDetailedHVACTSData("Detailed-HVAC"),
			ResultsSchema::RITimestepTSData("Timestep"),
			ResultsSchema::RIHourlyTSData("Hourly"),
			ResultsSchema::RIDailyTSData("Daily"),
			ResultsSchema::RIMonthlyTSData("Monthly"),
			ResultsSchema::RIRunPeriodTSData("RunPeriod");


		ResultsSchema::ResultsSchema() {
			tsEnabled = false;
			tsAndTabularEnabled = false;
		}

		ResultsSchema::~ResultsSchema() {

		}

		void ResultsSchema::setupOutputOptions() {
			int numberOfOutputSchemaObjects = InputProcessor::GetNumObjectsFound("Output:JSON");
			if (numberOfOutputSchemaObjects == 1) {
				try {
					FArray1D_string alphas(5);
					int numAlphas;
					FArray1D< Real64 > numbers(2);
					int numNumbers;
					int status;
					InputProcessor::GetObjectItem("Output:JSON", 1, alphas, numAlphas, numbers, numNumbers, status);

					if (numAlphas > 0) {
						std::string option = alphas(1);
						if (InputProcessor::SameString(option, "TimeSeries")) {
							tsEnabled = true;
						}
						else if (InputProcessor::SameString(option, "TimeSeriesAndTabular")) {
							tsEnabled = true;
							tsAndTabularEnabled = true;
						}
					}
				}
				catch (const std::runtime_error& error) {
					ShowFatalError(error.what());
				}
			}
		}

		bool ResultsSchema::timeSeriesEnabled() {
			return tsEnabled;
		}

		bool ResultsSchema::timeSeriesAndTabularEnabled() {
			return tsAndTabularEnabled;
		}

		void ResultsSchema::initializeRTSDataFrame(const int ReportFrequency, const FArray1D< RealVariableType > &RVariableTypes, const int NumOfRVariable, const int IndexType) {
			Reference< RealVariables > RVar;

			for (int Loop = 1; Loop <= NumOfRVariable; ++Loop) {
					RVar >>= RVariableTypes(Loop).VarPtr;
					auto & rVar(RVar());
					
					if (rVar.Report && rVar.ReportFreq == ReportFrequency && rVar.Stored)
					{
						Variable *var = new Variable(RVariableTypes(Loop).VarName, 
								ReportFrequency, RVariableTypes(Loop).IndexType, 
								RVariableTypes(Loop).ReportID,
								RVariableTypes(Loop).UnitsString);
						switch (ReportFrequency)
						{
						case -1:  // each time UpdatedataandReport is called
							if (IndexType == ZoneVar && RVariableTypes(Loop).IndexType == ZoneVar)
							{
								RIDetailedZoneTSData.setRDataFrameEnabled(true);
								RIDetailedZoneTSData.addVariable(var);
							}
							else if (IndexType == HVACVar && RVariableTypes(Loop).IndexType == HVACVar)
							{
								RIDetailedHVACTSData.setRDataFrameEnabled(true);
								RIDetailedHVACTSData.addVariable(var);
							}
							break;
						case 0:  // at 'EndTimeStepFlag'
							RITimestepTSData.setRDataFrameEnabled(true);
							RITimestepTSData.addVariable(var);
							break;
						case 1:  // at 'EndHourFlag'
							RIHourlyTSData.setRDataFrameEnabled(true);
							RIHourlyTSData.addVariable(var);
							break;
						case 2: // at 'EndDayFlag'
							RIDailyTSData.setRDataFrameEnabled(true);
							RIDailyTSData.addVariable(var);
							break;
						case 3:  // at end of month
							RIMonthlyTSData.setRDataFrameEnabled(true);
							RIMonthlyTSData.addVariable(var);
							break;
						case 4:  // once per environment 'EndEnvrnFlag'
							RIRunPeriodTSData.setRDataFrameEnabled(true);
							RIRunPeriodTSData.addVariable(var);
							break;
					}
				}
			}
		}

		void ResultsSchema::initializeITSDataFrame(const int ReportFrequency, const FArray1D< IntegerVariableType > &IVariableTypes, const int NumOfIVariable, const int IndexType) {
			Reference< IntegerVariables > IVar;

			for (int Loop = 1; Loop <= NumOfIVariable; ++Loop) {
				IVar >>= IVariableTypes(Loop).VarPtr;
				auto & iVar(IVar());
				if (iVar.Report && iVar.ReportFreq == ReportFrequency && iVar.Stored)
				{
					Variable *var = new Variable(IVariableTypes(Loop).VarName, ReportFrequency, 
								IVariableTypes(Loop).IndexType, 
								IVariableTypes(Loop).ReportID,
								IVariableTypes(Loop).UnitsString);
					switch (ReportFrequency)
					{
					case -1:  // each time UpdatedataandReport is called
						if (IndexType == ZoneVar && IVariableTypes(Loop).IndexType == ZoneVar)
						{
							RIDetailedZoneTSData.setIDataFrameEnabled(true);
							RIDetailedZoneTSData.addVariable(var);
						}
						else if (IndexType == HVACVar && IVariableTypes(Loop).IndexType == HVACVar)
						{
							RIDetailedZoneTSData.setIDataFrameEnabled(true);
							RIDetailedZoneTSData.addVariable(var);
						}
						break;
					case 0:  // at 'EndTimeStepFlag'
						RITimestepTSData.setIDataFrameEnabled(true);
						RITimestepTSData.addVariable(var);
						break;
					case 1:  // at 'EndHourFlag'
						RIHourlyTSData.setIDataFrameEnabled(true);
						RIHourlyTSData.addVariable(var);
						break;
					case 2: // at 'EndDayFlag'
						RIDailyTSData.setIDataFrameEnabled(true);
						RIDailyTSData.addVariable(var);
						break;
					case 3:  // at end of month
						RIMonthlyTSData.setIDataFrameEnabled(true);
						RIMonthlyTSData.addVariable(var);
						break;
					case 4:  // once per environment 'EndEnvrnFlag'
						RIRunPeriodTSData.setIDataFrameEnabled(true);
						RIRunPeriodTSData.addVariable(var);
						break;
					}
				}
			}

		}

		void ResultsSchema::writeTimeSeriesFiles()
		{

			if (OutputSchema->RIDetailedZoneTSData.rDataFrameEnabled() || OutputSchema->RIDetailedZoneTSData.iDataFrameEnabled())
				OutputSchema->RIDetailedZoneTSData.writeFile();

			// Output detailed HVAC time series data
			if (OutputSchema->RIDetailedHVACTSData.iDataFrameEnabled() || OutputSchema->RIDetailedHVACTSData.rDataFrameEnabled())
				OutputSchema->RIDetailedHVACTSData.writeFile();

			// Output timestep time series data
			if (OutputSchema->RITimestepTSData.iDataFrameEnabled() || OutputSchema->RITimestepTSData.rDataFrameEnabled())
				OutputSchema->RITimestepTSData.writeFile();

			// Output hourly time series data
			if (OutputSchema->RIHourlyTSData.iDataFrameEnabled() || OutputSchema->RIHourlyTSData.rDataFrameEnabled())
				OutputSchema->RIHourlyTSData.writeFile();

			// Output daily time series data
			if (OutputSchema->RIDailyTSData.iDataFrameEnabled() || OutputSchema->RIDailyTSData.rDataFrameEnabled())
				OutputSchema->RIDailyTSData.writeFile();

			// Output monthly time series data
			if (OutputSchema->RIMonthlyTSData.iDataFrameEnabled() || OutputSchema->RIMonthlyTSData.rDataFrameEnabled())
				OutputSchema->RIMonthlyTSData.writeFile();

			// Output run period time series data
			if (OutputSchema->RIRunPeriodTSData.iDataFrameEnabled() || OutputSchema->RIRunPeriodTSData.rDataFrameEnabled())
				OutputSchema->RIRunPeriodTSData.writeFile();
		}

		void ResultsSchema::writeFile()
		{
			std::string jsonfilename = "eplusout.json";
			std::ofstream jsonfile(jsonfilename);

			cJSON *_root, *_simRes, *outputVars;

			_root = cJSON_CreateObject();

			cJSON_AddItemToObject(_root, "SimulationResults", _simRes = cJSON_CreateObject());
			cJSON_AddItemToObject(_simRes, "Simulation", SimulationInformation.GetJSON());
			
			cJSON_AddItemToObject(_root, "OutputVariables", outputVars = cJSON_CreateObject());
			if (RIDetailedZoneTSData.iDataFrameEnabled() || RIDetailedZoneTSData.rDataFrameEnabled())
			{
				cJSON *arr;
				cJSON_AddItemToObject(outputVars, "Detailed-Zone", arr = cJSON_CreateArray());
				for (int i = 0; i < RIDetailedZoneTSData.variables().size(); i++)
					cJSON_AddItemToArray(arr, RIDetailedZoneTSData.variables()[i]->GetJSON());
			}
			if (RIDetailedZoneTSData.iDataFrameEnabled() || RIDetailedZoneTSData.rDataFrameEnabled())
			{
				cJSON *arr;
				cJSON_AddItemToObject(outputVars, "Detailed-Zone", arr = cJSON_CreateArray());
				for (int i = 0; i < RIDetailedZoneTSData.variables().size(); i++)
					cJSON_AddItemToArray(arr, RIDetailedZoneTSData.variables()[i]->GetJSON());
			}
			if (RIDetailedHVACTSData.iDataFrameEnabled() || RIDetailedHVACTSData.rDataFrameEnabled())
			{
				cJSON *arr;
				cJSON_AddItemToObject(outputVars, "Detailed-HVAC", arr = cJSON_CreateArray());
				for (int i = 0; i < RIDetailedHVACTSData.variables().size(); i++)
					cJSON_AddItemToArray(arr, RIDetailedHVACTSData.variables()[i]->GetJSON());
			}
			if (RITimestepTSData.iDataFrameEnabled() || RITimestepTSData.rDataFrameEnabled())
			{
				cJSON *arr;
				cJSON_AddItemToObject(outputVars, "TimeStep", arr = cJSON_CreateArray());
				for (int i = 0; i < RITimestepTSData.variables().size(); i++)
					cJSON_AddItemToArray(arr, RITimestepTSData.variables()[i]->GetJSON());
			}
			if (RIHourlyTSData.iDataFrameEnabled() || RIHourlyTSData.rDataFrameEnabled())
			{
				cJSON *arr;
				cJSON_AddItemToObject(outputVars, "Hourly", arr = cJSON_CreateArray());
				for (int i = 0; i < RIHourlyTSData.variables().size(); i++)
					cJSON_AddItemToArray(arr, RIHourlyTSData.variables()[i]->GetJSON());
			}
			if (RIDailyTSData.iDataFrameEnabled() || RIDailyTSData.rDataFrameEnabled())
			{
				cJSON *arr;
				cJSON_AddItemToObject(outputVars, "Daily", arr = cJSON_CreateArray());
				for (int i = 0; i < RIDailyTSData.variables().size(); i++)
					cJSON_AddItemToArray(arr, RIDailyTSData.variables()[i]->GetJSON());
			}
			if (RIMonthlyTSData.iDataFrameEnabled() || RIMonthlyTSData.rDataFrameEnabled())
			{
				cJSON *arr;
				cJSON_AddItemToObject(outputVars, "Monthly", arr = cJSON_CreateArray());
				for (int i = 0; i < RIMonthlyTSData.variables().size(); i++)
					cJSON_AddItemToArray(arr, RIMonthlyTSData.variables()[i]->GetJSON());
			}
			if (RIRunPeriodTSData.iDataFrameEnabled() || RIRunPeriodTSData.rDataFrameEnabled())
			{
				cJSON *arr;
				cJSON_AddItemToObject(outputVars, "RunPeriod", arr = cJSON_CreateArray());
				for (int i = 0; i < RIRunPeriodTSData.variables().size(); i++)
					cJSON_AddItemToArray(arr, RIRunPeriodTSData.variables()[i]->GetJSON());
			}
			
			if (jsonfile.is_open())	{
				jsonfile << cJSON_Print(_root);
				jsonfile.close();
			}

			cJSON_Delete(_root);
		}
	} // ResultsFramework

} // EnergyPlus
