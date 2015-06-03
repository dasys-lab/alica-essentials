/*
 * ProcessManagerRegistry.cpp
 *
 *  Created on: Feb 13, 2015
 *      Author: Stephan Opfer
 */

#include <RobotExecutableRegistry.h>
#include "ExecutableMetaData.h"
#include "RobotMetaData.h"
#include <SystemConfig.h>
#include <iostream>
#include "ConsoleCommandHelper.h"

namespace supplementary
{

	RobotExecutableRegistry::RobotExecutableRegistry()
	{
	}

	RobotExecutableRegistry::~RobotExecutableRegistry()
	{
		for (auto metaData : this->executableList)
		{
			delete metaData;
		}

		for (auto metaData : this->robotList)
		{
			delete metaData;
		}
	}

	bool RobotExecutableRegistry::getRobotName(int robotId, string& robotName)
	{
		for (auto robotMetaData : this->robotList)
		{
			if (robotMetaData->id == robotId)
			{
				robotName = robotMetaData->name;
				return true;
			}
		}

		robotName = "";
		return false;
	}

	bool RobotExecutableRegistry::robotExists(int robotId)
	{
		for (auto robotMetaData : this->robotList)
		{
			if (robotMetaData->id == robotId)
			{
				return true;
			}
		}
		return false;
	}

	bool RobotExecutableRegistry::robotExists(string robotName)
	{
		for (auto robotMetaData : this->robotList)
		{
			if (robotMetaData->name == robotName)
			{
				return true;
			}
		}
		return false;
	}

	bool RobotExecutableRegistry::getRobotId(string robotName, int& robotId)
	{
		for (auto robotMetaData : this->robotList)
		{
			if (robotMetaData->name == robotName)
			{
				robotId = robotMetaData->id;
				return true;
			}
		}

		robotId = 0;
		return false;
	}

	void RobotExecutableRegistry::addRobot(string robotName, int robotId)
	{
		this->robotList.push_back(new RobotMetaData(robotName, robotId));
	}

	/**
	 * Adds a robot with its configured id, if it exists. Otherwise it generates a new unique id.
	 * This method allows testing with systems, which are not in the Globals.conf,
	 * i.d., are no official robots.
	 */
	int RobotExecutableRegistry::addRobot(string robotName)
	{
		SystemConfig* sc = SystemConfig::getInstance();
		int robotId;
		try
		{
			robotId = (*sc)["Globals"]->get<int>("Globals.Team", robotName.c_str(), "ID", NULL);
		}
		catch (runtime_error& e)
		{
			robotId = 0;
			bool idExists;

			do
			{
				idExists = false;
				robotId++;
				for (auto entry : this->robotList)
				{
					if (entry->id == robotId)
					{
						idExists = true;
						break;
					}
				}
			} while (idExists);
			cout << "PM Registry: Warning! Adding unknown robot " << robotName << " with ID " << robotId << "!" << endl;
		}

		this->addRobot(robotName, robotId);
		return robotId;
	}

	const vector<RobotMetaData*>& RobotExecutableRegistry::getRobots() const
	{
		return this->robotList;
	}

	bool RobotExecutableRegistry::getExecutableName(int execId, string& execName)
	{
		for (auto execMetaData : this->executableList)
		{
			if (execMetaData->id == execId)
			{
				execName = execMetaData->name;
				return true;
			}
		}

		execName = "";
		return false;
	}

	bool RobotExecutableRegistry::getExecutableId(string execName, int& execId)
	{
		for (auto execMetaData : this->executableList)
		{
			if (execMetaData->name == execName)
			{
				execId = execMetaData->id;
				return true;
			}
		}

		execId = 0;
		return false;
	}

	bool RobotExecutableRegistry::executableExists(int execId)
	{
		for (auto execMetaData : this->executableList)
		{
			if (execMetaData->id == execId)
			{
				return true;
			}
		}

		return false;
	}

	bool RobotExecutableRegistry::executableExists(string execName)
	{
		for (auto execMetaData : this->executableList)
		{
			if (execMetaData->name == execName)
			{
				return true;
			}
		}

		return false;
	}

	/**
	 * This method registers the given executable, if it is listed in the Processes.conf file.
	 * @param execName
	 * @return -1, if the executable is not registered, due to some error. Otherwise, it returns the registered id.
	 */
	int RobotExecutableRegistry::addExecutable(string execName)
	{
		if (this->executableExists(execName))
		{
			cerr << "RobotExecutableRegistry: The executable '" << execName << "' is already registered!" << endl;
			return -1;
		}

		SystemConfig* sc = SystemConfig::getInstance();
		int execId;
		string absExecName;
		string processMode;

		try
		{
			execId = (*sc)["Processes"]->get<int>("Processes.ProcessDescriptions", execName.c_str(), "id", NULL);
			processMode = (*sc)["Processes"]->get<string>("Processes.ProcessDescriptions", execName.c_str(), "mode", NULL);
		}
		catch (runtime_error& e)
		{
			cerr << "PM-Registry: Cannot add executable '" << execName << "', because of faulty values in Processes.conf!" << endl;
			return -1;
		}

		string cmd = "catkin_find --libexec " + execName;
		absExecName = supplementary::ConsoleCommandHelper::exec(cmd.c_str());

		if (absExecName.length() > 1)
		{
			absExecName = absExecName.substr(0, absExecName.length() - 1);
			absExecName = absExecName + "/" + execName;
		}

		ExecutableMetaData* execMetaData = new ExecutableMetaData(execName, execId, processMode, absExecName);
		auto paramSets = (*sc)["Processes"]->tryGetNames("NONE", "Processes.ProcessDescriptions", execName.c_str(), "paramSets", NULL);
		if (paramSets->size() > 1 || paramSets->at(0) != "NONE")
		{
			for (string paramSetKeyString : (*paramSets))
			{
				try
				{
					int paramSetKey = stoi(paramSetKeyString);
					auto paramSetValues = (*sc)["Processes"]->getList<string>("Processes.ProcessDescriptions", execName.c_str(), "paramSets",
																				paramSetKeyString.c_str(), NULL);

					// first param is always the executable name
					vector<char*> currentParams;
					if (absExecName.length() > 1)
					{
						currentParams.push_back(strdup(absExecName.c_str()));
					}
					else
					{
						currentParams.push_back(strdup(execName.c_str()));
					}
					// transform the system config params to vector of char*, for c-compatibility.
					for (string param : paramSetValues)
					{
						char * tmp = new char[param.size() + 1];
						strcpy(tmp, param.c_str());
						tmp[param.size() + 1] = '\0';
						currentParams.push_back(tmp);
					}
					currentParams.push_back(nullptr);

					execMetaData->addParameterSet(paramSetKey, currentParams);
				}
				catch (exception & e)
				{
					cerr << "RobotExecutableRegistry: Unable to parse parameter set \"" << paramSetKeyString << "\" of process \"" << execName << "\""
							<< endl;
					cerr << e.what() << endl;
				}
			}
		}
		else
		{
			vector<char*> currentParams;
			if (absExecName.length() > 1)
			{
				currentParams.push_back(strdup(absExecName.c_str()));
			}
			else
			{
				currentParams.push_back(strdup(execName.c_str()));
			}
			currentParams.push_back(nullptr);
			execMetaData->addParameterSet(0, currentParams);
		}

		//cout << (*execMetaData) << endl;
		this->executableList.push_back(execMetaData);
		return execId;
	}

	/**
	 * This method is for copying meta data from an entry into a real managed executable.
	 * Don't change anything in the returns object.
	 * @param execName is the name of the demanded entry.
	 * @return The demanded entry, if it exists. nullptr, otherwise.
	 */
	ExecutableMetaData
	const * const RobotExecutableRegistry::getExecutable(string execName) const
	{
		for (auto execEntry : this->executableList)
		{
			if (execEntry->name == execName)
			{
				return execEntry;
			}
		}
		return nullptr;
	}

	/**
	 * This method is for copying meta data from an entry into a real managed executable.
	 * Don't change anything in the returns object.
	 * @param execId is the id of the demanded entry.
	 * @return The demanded entry, if it exists. nullptr, otherwise.
	 */
	ExecutableMetaData
	const * const RobotExecutableRegistry::getExecutable(int execId) const
	{
		for (auto execEntry : this->executableList)
		{
			if (execEntry->id == execId)
			{
				return execEntry;
			}
		}
		return nullptr;
	}

	/**
	 * For accessing the internal data structure of executable meta data entries.
	 * @return The internal data structure of executable meta data entries.
	 */
	const vector<ExecutableMetaData*>& RobotExecutableRegistry::getExecutables() const
	{
		return this->executableList;
	}

} /* namespace supplementary */
