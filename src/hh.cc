
/********************************************************************
This code is based on Approximate frequent items in a data stream
G. Cormode 2002, 2003,2005

Last modified: December 2016
*********************************************************************/
#include "lossycount.h"
#include "losum.h"
#include "alosum.h"
#include "ccfc.h"
#include "countmin.h"
#include <fstream>

/******************************************************************/

#ifdef _MSC_VER
#include <windows.h>
#endif

class Stats
{
public:
	Stats() : dU(0.0), dQ(0.0), dP(0.0), dR(0.0), dF(0.0), dF2(0.0) {}

	double dU, dQ;
	double dP, dR, dF, dF2;
	std::multiset<double> P, R, F, F2;
};

void usage()
{
	std::cerr
		<< "Usage: graham\n"
		<< "  -np		number of packets\n"
		<< "  -r		number of runs\n"
		<< "  -phi		phi\n"
		<< "  -d		depth\n"
		<< "  -g		granularity\n"
		<< "  -gamma    DIM-SUM coefficient\n"
		<< "  -z    skew\n"
		<< std::endl;
}

void StartTheClock(uint64_t& s)
{
#ifdef _MSC_VER
	FILETIME ft;
    LARGE_INTEGER li;

	GetSystemTimeAsFileTime(&ft);
	li.LowPart = ft.dwLowDateTime;
	li.HighPart = ft.dwHighDateTime;
	s = (uint64_t) (li.QuadPart / 10000);
#else
	struct timeval tv;
	gettimeofday(&tv, 0);
	s = (1000 * tv.tv_sec) + (tv.tv_usec / 1000);
#endif
}

// returns milliseconds.
uint64_t StopTheClock(uint64_t s)
{
#ifdef _MSC_VER
    FILETIME ft;
    LARGE_INTEGER li;
    uint64_t t;

	GetSystemTimeAsFileTime(&ft);
	li.LowPart = ft.dwLowDateTime;
	li.HighPart = ft.dwHighDateTime;
	t = (uint64_t) (li.QuadPart / 10000);
	return t - s;
#else
	struct timeval tv;
	gettimeofday(&tv, 0);
	return (1000 * tv.tv_sec) + (tv.tv_usec / 1000) - s;
#endif
}

void CheckOutput(std::map<uint32_t, uint32_t>& res, uint64_t thresh, size_t hh, Stats& S, const std::vector<uint32_t>& exact)
{
	if (res.empty())
	{
		S.F.insert(0.0);
		S.F2.insert(0.0);
		S.P.insert(100.0);
		S.dP += 100.0;

		if (hh == 0)
		{
			S.R.insert(100.0);
			S.dR += 100.0;
		}
		else
			S.R.insert(0.0);

		return;
	}

	size_t correct = 0;
	size_t claimed = res.size();
	size_t falsepositives = 0;
	double e = 0.0, e2 = 0.0;

	std::map<uint32_t, uint32_t>::iterator it;
	for (it = res.begin(); it != res.end(); ++it)
	{
		if (exact[it->first] >= thresh)
		{
			++correct;
			uint32_t ex = exact[it->first];
			double diff = (ex > it->second) ? ex - it->second : it->second - ex;
			e += diff / ex;
		}
		else
		{
			++falsepositives;
			uint32_t ex = exact[it->first];
			double diff = (ex > it->second) ? ex - it->second : it->second - ex;
			e2 += diff / ex;
		}
	}

	if (correct != 0)
	{
		e /= correct;
		S.F.insert(e);
		S.dF += e;
	}
	else
		S.F.insert(0.0);

	if (falsepositives != 0)
	{
		e2 /= falsepositives;
		S.F2.insert(e2);
		S.dF2 += e2;
	}
	else
		S.F2.insert(0.0);

	double r = 100.0;
	if (hh != 0) r = 100.0 *((double) correct) / ((double) hh);

	double p = 100.0 *((double) correct) / ((double) claimed);

	S.R.insert(r);
	S.dR += r;
	S.P.insert(p);
	S.dP += p;
}

void PrintTimes(char* title, std::vector<uint64_t> times) {
	std::cout << title;
	for (auto const& t : times) {
		std::cout << "\t" << t;
	}
	std::cout << std::endl;
}

void PrintOutput(char* title, size_t size, const Stats& S, size_t u32NumberOfPackets)
{
	double p5th = -1.0, p95th = -1.0, r5th = -1.0, r95th = -1.0, f5th = -1.0, f95th = -1.0, f25th = -1.0, f295th = -1.0;
	size_t i5, i95;
	std::multiset<double>::const_iterator it;

	if (! S.P.empty())
	{
		it = S.P.begin();
		i5 = (size_t) (S.P.size() * 0.05);
		for (size_t i = 0; i < i5; ++i) ++it;
		p5th = *it;
		i95 = (size_t) (S.P.size() * 0.95);
		for (size_t i = 0; i < (i95 - i5); ++i) ++it;
		p95th = *it;
	}

	if (! S.R.empty())
	{
		it = S.R.begin();
		i5 = S.R.size() * 0.05;
		for (size_t i = 0; i < i5; ++i) ++it;
		r5th = *it;
		i95 = S.R.size() * 0.95;
		for (size_t i = 0; i < (i95 - i5); ++i) ++it;
		r95th = *it;
	}

	if (! S.F.empty())
	{
		it = S.F.begin();
		i5 = S.F.size() * 0.05;
		for (size_t i = 0; i < i5; ++i) ++it;
		f5th = *it;
		i95 = S.F.size() * 0.95;
		for (size_t i = 0; i < (i95 - i5); ++i) ++it;
		f95th = *it;
	}

	if (! S.F2.empty())
	{
		it = S.F2.begin();
		i5 = S.F2.size() * 0.05;
		for (size_t i = 0; i < i5; ++i) ++it;
		f25th = *it;
		i95 = S.F2.size() * 0.95;
		for (size_t i = 0; i < (i95 - i5); ++i) ++it;
		f295th = *it;
	}

	printf("%s\t%1.2f\t%d\t%1.2f\t%1.2f\t%1.2f\t%1.2f\t%1.2f\t%1.2f\t%1.2f\t%1.2f\t%1.2f\t%1.2f\t%1.2f\t%1.2f\n",
		title, u32NumberOfPackets / S.dU, size,
		S.dR / S.R.size(), r5th, r95th,
		S.dP / S.P.size(), p5th, p95th,
		S.dF / S.F.size(), f5th, f95th,
		S.dF2 / S.F2.size(), f25th, f295th
	);
}

size_t RunExact(uint64_t thresh, std::vector<uint32_t>& exact)
{
	size_t hh = 0;

	for (size_t i = 0; i < exact.size(); ++i)
		if (exact[i] >= thresh) ++hh;

	return hh;
}

/******************************************************************/

int main(int argc, char **argv) 
{
	size_t stNumberOfPackets = 10000000;
	size_t stRuns = 20;
	double dPhi = 0.001;//0.000001;//0.001;
	double gamma = 4.;
	bool gammaDefined = false;
	uint32_t u32Depth = 4;
	uint32_t u32Granularity = 8;
	std::string file = "";
	bool timeLaspe = false;
	double dSkew = 1.0;
	for (int i = 1; i < argc; ++i)
	{
		if (strcmp(argv[i], "-np") == 0)
		{
			i++;
			if (i >= argc)
			{
				std::cerr << "Missing number of packets." << std::endl;
				return -1;
			}
			stNumberOfPackets = atoi(argv[i]);
		}
		else if (strcmp(argv[i], "-r") == 0)
		{
			i++;
			if (i >= argc)
			{
				std::cerr << "Missing number of runs." << std::endl;
				return -1;
			}
			stRuns = atoi(argv[i]);
		}
		else if (strcmp(argv[i], "-d") == 0)
		{
			i++;
			if (i >= argc)
			{
				std::cerr << "Missing depth." << std::endl;
				return -1;
			}
			u32Depth = atoi(argv[i]);
		}
		else if (strcmp(argv[i], "-g") == 0)
		{
			i++;
			if (i >= argc)
			{
				std::cerr << "Missing granularity." << std::endl;
				return -1;
			}
			u32Granularity = atoi(argv[i]);
		}
		else if (strcmp(argv[i], "-phi") == 0)
		{
			i++;
			if (i >= argc)
			{
				std::cerr << "Missing phi." << std::endl;
				return -1;
			}
			dPhi = atof(argv[i]);
		}
		else if (strcmp(argv[i], "-f") == 0)
		{
			i++;
			if (i >= argc)
			{
				std::cerr << "Missing file name." << std::endl;
				return -1;
			}
			file = std::string(argv[i]);
		}
		else if (strcmp(argv[i], "-t") == 0)
		{
			timeLaspe = true;
		}
		else if (strcmp(argv[i], "-gamma") == 0)
		{
			i++;
			if (i >= argc)
			{
				std::cerr << "Missing gamma." << std::endl;
				return -1;
			}
			gamma = atof(argv[i]);
			gammaDefined = true;

		}
		else if (strcmp(argv[i], "-z") == 0)
		{
			i++;
			if (i >= argc)
			{
				std::cerr << "Missing skew parameter." << std::endl;
				return -1;
			}
			dSkew = atof(argv[i]);
		}
		else if (strcmp(argv[i], "-measure_time_granularity") == 0) {
			uint64_t s;
			StartTheClock(s);
			uint64_t t = 0;
			while (t == 0) {
				t = StopTheClock(s);
			}
			std::cout << "Time granularity is " << t << std::endl;
		}
		else
		{
			usage();
			return -1;
		}
	}

	uint32_t u32Width = 2.0 / dPhi;

	prng_type * prng;
	prng=prng_Init(44545,2);
	int64_t a = (int64_t) (prng_int(prng)% MOD);
	int64_t b = (int64_t) (prng_int(prng)% MOD);
	prng_Destroy(prng);

	uint32_t u32DomainSize = 1048575;
	std::vector<uint32_t> exact(u32DomainSize + 1, 0);
	Stats SLS, SCMH, SCCFC, SALS, SLCL;
	std::vector<uint64_t> TLS, TCMH, TCCFC, TALS, TLCL;
	CMH_type* cmh = CMH_Init(u32Width, u32Depth, 32, u32Granularity);
	CCFC_type* ccfc = CCFC_Init(u32Width, u32Depth, 32, u32Granularity);
	LCL_type* lcl = LCL_Init(dPhi);
	LS_type* ls = LS_Init(dPhi, gamma);
	ALS_type* als = ALS_Init(dPhi, gamma);

	std::vector<uint32_t> data;
	std::vector<uint32_t> values;
	Tools::Random r = Tools::Random(0xF4A54B);
	Tools::PRGZipf zipf = Tools::PRGZipf(0, u32DomainSize, dSkew, &r);

	size_t stCount = 0;
	if (file != "") {
		uint64_t total = 0;
		std::ifstream f(file);
		int id, length;
		while (f >> id >> length) {
			assert(length > 0);
			if (total < 0) {
				std::cerr << "Why is total negative? " << total<<std::endl;
				break;
			}
			if ((total + abs(length)) >= 0x7FFFFFFE) {
				std::cerr <<  "Error! total number of bytes is " << total << " and trying to add " << length << std::endl;
				break;
			}
			data.push_back(id);
			values.push_back(length);
			total += length;
		}
		std::cerr << "Finished loading file. Total number of bytes: " << total << std::endl;
	}
	else {
		for (int i = 0; i < stNumberOfPackets; ++i)
		{
			++stCount;
			if (stCount % 500000 == 0)
				std::cerr << stCount << std::endl;
			uint32_t v = zipf.nextLong();
			uint32_t value = hash31(a, b, v) & u32DomainSize;
			if (value > 0) {
				data.push_back(value);
				values.push_back(1);
			}
			else {
				data.push_back(-value);
				values.push_back(1);
			}
		}
	}
	size_t stRunSize = data.size() / stRuns;
	size_t stStreamPos = 0;
	uint64_t nsecs;
	uint64_t t;
	long long total = 0;
	for (size_t run = 1; run <= stRuns; ++run) // stRuns
	{
		bool stop = false;
		for (size_t i = stStreamPos; i < stStreamPos + stRunSize; ++i)
		{
			assert(values[i] > 0);
			total += abs((int)values[i]);
			if (total >= 0x7FFFFFFF) {
				std::cerr << "Error! Total number of bytes is " << total << std::endl;
				stop = true;
				break;
			}
			exact[data[i]]+=values[i];
			if (exact[data[i]] > 0x7FFFFFFF) {
				std::cerr << "Strange. Value is too large " <<exact[data[i]]<< " after addding "<<values[i]<< std::endl;
			}
			
		}
		if (stop) {
			break;
		}
		if (!gammaDefined) {
			StartTheClock(nsecs);
			for (size_t i = stStreamPos; i < stStreamPos + stRunSize; ++i)
			{
				CMH_Update(cmh, data[i], values[i]);
			}
			SCMH.dU += t = StopTheClock(nsecs);
			TCMH.push_back(t);

			StartTheClock(nsecs);
			for (size_t i = stStreamPos; i < stStreamPos + stRunSize; ++i)
			{
				LCL_Update(lcl, data[i], values[i]);
			}
			SLCL.dU += t = StopTheClock(nsecs);
			TLCL.push_back(t);
		}
		StartTheClock(nsecs);
		for (size_t i = stStreamPos; i < stStreamPos + stRunSize; ++i)
		{
			LS_Update(ls, data[i],values[i]);
		}
		SLS.dU += t = StopTheClock(nsecs);
		TLS.push_back(t);
		
		StartTheClock(nsecs);
		for (size_t i = stStreamPos; i < stStreamPos + stRunSize; ++i)
		{
			ALS_Update(als, data[i], values[i]);
		}
		SALS.dU += t = StopTheClock(nsecs);
		TALS.push_back(t);
		StartTheClock(nsecs);
		for (size_t i = stStreamPos; i < stStreamPos + stRunSize; ++i)
		{
			CCFC_Update(ccfc,data[i],values[i]);      
		}
		SCCFC.dU += t = StopTheClock(nsecs);
		TCCFC.push_back(t);

		uint64_t thresh = static_cast<uint64_t>(floor(dPhi*total)+1);//floor(dPhi * run * stRunSize));
		std::cerr << "total "<<total<<" thresh " << thresh << std::endl;
		size_t hh = RunExact(thresh, exact);
		std::cerr << "Run: " << run << ", Exact: " << hh << std::endl;

		std::map<uint32_t, uint32_t> res;
		
		if (!gammaDefined) {
			StartTheClock(nsecs);
			res = CMH_FindHH(cmh, thresh);
			SCMH.dQ += StopTheClock(nsecs);
			CheckOutput(res, thresh, hh, SCMH, exact);
			StartTheClock(nsecs);
			res = LCL_Output(lcl, thresh);
			SLCL.dQ += StopTheClock(nsecs);
			CheckOutput(res, thresh, hh, SLCL, exact);
			StartTheClock(nsecs);
			res = CCFC_Output(ccfc, thresh);
			SCCFC.dQ += StopTheClock(nsecs);
			CheckOutput(res, thresh, hh, SCCFC, exact);

		}
		StartTheClock(nsecs);
		res = LS_Output(ls, thresh);
		SLS.dQ += StopTheClock(nsecs);
		CheckOutput(res, thresh, hh, SLS, exact);

		
		StartTheClock(nsecs);
		res = ALS_Output(als, thresh);
		SLS.dQ += StopTheClock(nsecs);
		CheckOutput(res, thresh, hh, SALS, exact);
		
		stStreamPos += stRunSize;
	}
	if (timeLaspe) {
		PrintTimes("IM-SUM", TALS);
		PrintTimes("DIM-SUM", TLS);
		PrintTimes("CM", TCMH);
		PrintTimes("CS", TCCFC);
		PrintTimes("SSH", TLCL);
	}
	else {
		printf("\nMethod\tUpdates/ms\tSpace\tRecall\t5th\t95th\tPrecis\t5th\t95th\tFreq RE\t5th\t95th\n");
		stNumberOfPackets = data.size();
		PrintOutput("ALS", ALS_Size(als), SALS, stNumberOfPackets);
		PrintOutput("LS", LS_Size(ls), SLS, stNumberOfPackets);
		if (!gammaDefined) {
			PrintOutput("CMH", CMH_Size(cmh), SCMH, stNumberOfPackets);
			PrintOutput("CCFC", CCFC_Size(ccfc), SCCFC, stNumberOfPackets);
			PrintOutput("SSH", LCL_Size(lcl), SLCL, stNumberOfPackets);
		}
	}
	CMH_Destroy(cmh);
	LCL_Destroy(lcl);  
	LS_Destroy(ls);
	ALS_Destroy(als);
	CCFC_Destroy(ccfc);

	printf("\n");
	return 0;
}
