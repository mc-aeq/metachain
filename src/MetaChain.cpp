/*********************************************************************
* Copyright (c) 2017 TCT DEVs	                                     *
* Distributed under the GPLv3.0 software license					 *
* contact us before using our code									 *
**********************************************************************/

#include "MetaChain.h"

#include <atomic>
#include <curl/curl.h>
#include <boost/filesystem/operations.hpp>

#include "SimpleIni.h"
#include "scheduler.h"
#include "functions.h"
#include "network/NetworkManager.h"
#include "logger.h"
#include "crypto/sha3.h"
#include "io/zip/unzip.h"

extern std::atomic<bool> sabShutdown;
extern std::atomic<bool> sabReboot;

MetaChain::MetaChain() :
	m_pNetworkManager( NULL ),
	m_iVersionTicksTillUpdate(0),
	m_bAutoUpdate(true)
{
}

MetaChain& MetaChain::getInstance()
{
	static MetaChain instance;
	return instance;
}

bool MetaChain::initialize(CSimpleIniA* iniFile, boost::filesystem::path pathExecutable)
{
	m_pathExecutable = pathExecutable;

	// read autoupdate settings
	m_iVersionTicksTillUpdate = iniFile->GetLongValue("autoupdate", "ticks_until_update_triggered", 10);
	m_bAutoUpdate = iniFile->GetBoolValue("autoupdate", "enable", true);
	m_strCDNUrl = iniFile->GetValue("autoupdate", "cdn_url", "https://cdn.tct.io/");
	m_pathTmp = iniFile->GetValue("autoupdate", "tmp_dir", "tmp");
	if (m_pathTmp.is_relative())
		m_pathTmp = boost::filesystem::current_path() / iniFile->GetValue("autoupdate", "tmp_dir", "tmp");

	// do a autoupdate call upon start if requested
	if (iniFile->GetBoolValue("autoupdate", "autoupdate_on_start", true))
	{
		// if the doAutoUpdate() function returns true, a update was successfully made.
		// we skip the rest of the initialization function since we need to start the new version
		if (doAutoUpdate())
			return false;
	}

	// start the lightweight scheduling thread
	CScheduler::Function serviceLoop = boost::bind(&CScheduler::serviceQueue, &m_scheduler);
	m_threadGroup.create_thread(boost::bind(&TraceThread<CScheduler::Function>, "scheduler", serviceLoop));

	// create the block storage backends, check their integrity and check if no other instance is running
	m_pStorageManager = new StorageManager(this);
	if (!m_pStorageManager->initialize(iniFile))
		return false;

	// create a new network manager
	m_pNetworkManager = new NetworkManager(this);

	// initializing the network manager
	if (!m_pNetworkManager->initialize(iniFile))
		return false;

	// finally everything is initialized and we print our copyright info
	LicenseInfo();

	// everything is fine
	return true;
}

void MetaChain::LicenseInfo()
{
	LOG("-------------------------------", "");
	LOG("Copyright (C) 2017, TrustChainTechnologies.io", "");
	LOG("Please join our forums at https://forum.trustchaintechnologies.io and if you have questions you can also contact our support team via support@trustchaintechnologies.io", "");
	LOG("If you want to contribute to this development, you can always check out the sources at https://github.com/TrustChainTechnologies/metachain", "");
	LOG("This is an experimental software distributed under the GPL Version 3, read the LICENSE file for further information.", "");
	LOG("-------------------------------", "");
}

void MetaChain::incrementNewerVersionTicker()
{
	m_usNewerVersionTicker++;

	// after X ticks, we read the ini value if we auto update. if not, we close the node. if we have autoupdate configured, we connect to a CDN and download the latest binary and try to restart the node smoothly
	if (m_usNewerVersionTicker >= m_iVersionTicksTillUpdate)
	{
		if (!m_bAutoUpdate)
		{
			// if we don't want to auto update, we close the node due to incompability
			sabShutdown = true;
		}
		else
			doAutoUpdate();
	}	
}

static size_t curlWriteCallback(void *contents, size_t size, size_t nmemb, void *userp)
{
	((std::string*)userp)->append((char*)contents, size * nmemb);
	return size * nmemb;
}

// auto update function that returns false if nothing was updated and true if something was updated
// when true is returned, the node needs to restart in order for changes to take affect
bool MetaChain::doAutoUpdate()
{
	if (m_strCDNUrl == "")
		return false;

	LOG("Checking for newer Version at: " + m_strCDNUrl, "AU");

	// first fetch the current version number from the CDN	
	CURL *curl = curl_easy_init();
	if (curl)
	{
		std::string strCurrentVersion;
		curl_easy_setopt(curl, CURLOPT_URL, (m_strCDNUrl + "/" + AU_VERSION_FILE).c_str());
		curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0);
		curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, curlWriteCallback);
		curl_easy_setopt(curl, CURLOPT_WRITEDATA, &strCurrentVersion);
		CURLcode res = curl_easy_perform(curl);
		curl_easy_cleanup(curl);

		if (res != CURLE_OK)
		{
			LOG_ERROR("Error receiving version number: " + (std::string)curl_easy_strerror(res), "AU");
			return false;
		}

		// remove tags that might be around and then convert to a number for version comparing
		boost::trim_if(strCurrentVersion, boost::is_any_of("\n\r "));

		// convert to number
		uint32_t currentVersion = 0;
		try
		{
			currentVersion = boost::lexical_cast<uint32_t>(strCurrentVersion);
		}
		catch (const boost::bad_lexical_cast &e)
		{
			LOG_ERROR("Can't convert version number to comparable format: (" + strCurrentVersion + ") " + (std::string)e.what(), "AU");
			return false;
		}

		// compare version numbers
		if (currentVersion != g_cuint32tVersion)
		{
			// get the newest version
			LOG("New version found, updating", "AU");

			// assembling the download uri
			std::string strURI = m_strCDNUrl + "/";
#ifdef _WIN32
			strURI += "windows/";
#else
			strURI += "linux/";
#endif
#ifdef _X86_64
			strURI += "x86_64/";
#else
			strURI += "i386/";
#endif
			strURI += strCurrentVersion + "/";

			// make sure we have a clean tmp directory
			try
			{
				boost::filesystem::remove_all(m_pathTmp);
				boost::filesystem::create_directory(m_pathTmp);
			}
			catch (const boost::filesystem::filesystem_error& e)
			{
				LOG_ERROR("Couldn't clean tmp directory for safe processing: " + (std::string)e.what(), "AU");
				return false;
			}

			// download the files
			if (!downloadFile(strURI + "check.sum", (m_pathTmp / AU_CHECKSUM_FILE).string()))
				return false;
			if (!downloadFile(strURI + "node.zip", (m_pathTmp / AU_NODE_FILE).string()))
				return false;

			// calc and check the hashsum
			LOG("Checking checksums", "AU");
			SHA3 crypto;
			std::ifstream ifs((m_pathTmp / AU_CHECKSUM_FILE).string());
			std::string strChecksum((std::istreambuf_iterator<char>(ifs)),	(std::istreambuf_iterator<char>()));
			boost::trim_if(strChecksum, boost::is_any_of("\n\r "));
			ifs.close();
			if (crypto.to_string(crypto.hashFile((m_pathTmp / AU_NODE_FILE).string(), SHA3::HashType::DEFAULT, SHA3::HashSize::SHA3_512), SHA3::HashSize::SHA3_512) != strChecksum)
			{
				LOG_ERROR("Checksums don't match", "AU");
				return false;
			}
			LOG("Checksums match, continuing updating", "AU");

			// extract content from the zip file
			LOG("Extracting ZIP file", "AU");
			ZIP::Unzip zipFile;
			zipFile.open( (m_pathTmp / AU_NODE_FILE).string().c_str() );
			auto filenames = zipFile.getFilenames();
			for (auto it = filenames.begin(); it != filenames.end(); it++)
			{
				std::ofstream outFile((m_pathTmp / *it).string(), std::ofstream::binary);

				zipFile.openEntry((*it).c_str());
				zipFile >> outFile;

				outFile.close();
			}
			zipFile.close();
			boost::filesystem::remove(m_pathTmp / AU_NODE_FILE);
			boost::filesystem::remove(m_pathTmp / AU_CHECKSUM_FILE);

			LOG("Replacing old files with new files (except configs)", "AU");
			boost::filesystem::path pathRoot(m_pathExecutable);
			pathRoot.remove_filename();

			// move the executable to a backup location
			if (boost::filesystem::exists(pathRoot / AU_NODE_BACKUPFILE))
				boost::filesystem::remove(pathRoot / AU_NODE_BACKUPFILE);
			boost::filesystem::rename(m_pathExecutable, pathRoot / AU_NODE_BACKUPFILE);			

			// copy all extracted files to our local path
			for (boost::filesystem::directory_iterator file(m_pathTmp); file != boost::filesystem::directory_iterator(); file++)
			{
				try
				{
					boost::filesystem::rename(file->path(), pathRoot / file->path().filename());
				}
				catch (const boost::filesystem::filesystem_error& e)
				{
					LOG_ERROR("Couldn't move updated file sources: " + (std::string)e.what(), "AU");
					return false;
				}
			}

			// the update process is now completed. the function that called this function needs to handle the shutdown and restart of the updated version
			LOG("Autoupdate process completed. Restarting node to finish the updating process... ", "AU");
			sabShutdown = true;	// shutdown the thread that runs the MC
			sabReboot = true;	// reboot the MC Node

			return true;
		}
		else
		{
			LOG("No new version found, continuing work", "AU");
			return false;
		}
	}
	else
	{
		LOG_ERROR("Error initializing curl, AutoUpdate aborted", "AU");
		return false;
	}
}

bool MetaChain::downloadFile(std::string strFrom, std::string strTo)
{
	FILE *fp = fopen(strTo.c_str(), "wb");
	if (!fp)
	{
		LOG_ERROR("Couldn't open " + strTo + " file for writing", "DL");
		return false;
	}

	CURL *curl = curl_easy_init();
	if (curl)
	{
		curl_easy_setopt(curl, CURLOPT_URL, strFrom.c_str());
		curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0);
		curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, NULL);
		curl_easy_setopt(curl, CURLOPT_WRITEDATA, fp);
		CURLcode res = curl_easy_perform(curl);
		curl_easy_cleanup(curl);
		fclose(fp);

		if (res != CURLE_OK)
		{
			LOG_ERROR("Couldn't download from " + strFrom + " and store it to " + strTo, "DL");
			return false;
		}
		else
			return true;
	}
	else
	{
		fclose(fp);
		LOG_ERROR("Error initializing curl, download aborted", "DL");
		return false;
	}
}