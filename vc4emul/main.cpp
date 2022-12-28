
#include "Processor.hpp"
#include "Device.hpp"
#include "Log.hpp"
#include "Memory.hpp"

#include <iostream>
#include <iterator>
#include <fstream>
#include <vector>
#include <string>

static Device *getDevice(const char *name, bool verbose) {
	auto devices = Device::getDeviceTypes();
	assert(devices.size() > 0);
	for (unsigned int i = 0; i < devices.size(); i++) {
		if (verbose)
			std::cout << "Device: " << devices[i]->getName()
				  << std::endl;
		if (std::string(devices[i]->getName()) == name) {
			return devices[i]->clone();
		}
	}
	return NULL;
}

static bool createSimulator(Processor **processor, std::vector<Device*>& devices,
			    bool verbose) {
	// Create the processor
	auto processors = Processor::getProcessorTypes();
	assert(processors.size() > 0);
	*processor = NULL;
	for (unsigned int i = 0; i < processors.size(); i++) {
		if (processors[i]->getName() == "VideoCore IV") {
			*processor = processors[i]->clone();
			break;
		}
	}
	if (*processor == NULL) {
		std::cerr << "VideoCore IV processor not available!" << std::endl;
		return false;
	}
	// Create the devices
	Device *dram = getDevice("BCM2835", verbose);
	if (dram == NULL) {
		std::cerr << "DRAM device not available!" << std::endl;
		return false;
	}
	devices.push_back(dram);
	return true;
}

static uintptr_t parse_uintptr(const char* str,bool& ok) {
	ok = false;
	if (!str) return 0;
	while (isspace(*str)) ++str;
	if (str[0]=='0' && str[1]=='x') {
		ok = true;
		return strtoll(str+2,nullptr,16);
	}
	ok = true;
	return strtoll(str,nullptr,10);
}

static std::vector<unsigned char> read_all(std::ifstream& is) {
	std::vector<unsigned char> buffer(std::istreambuf_iterator<char>(is), {});
	return buffer;
}

int main(int argc, char **argv) {
	
	int i = 1, first_pos_arg = 1, remaining_args;
	bool logging = false;
	bool verbose = false;
	
	if (argc <= 1)
		goto usage;
	
	while (true)
	  {
	    if (strcmp (argv[i], "-log") == 0)
	      logging = true;
	    else if (strcmp (argv[i], "-verbose") == 0)
	      verbose = true;
	    else
	      break;
	    
	    i++;
	  }

	first_pos_arg = i;
	remaining_args = argc - first_pos_arg;

	// Parse the input parameters
	if (remaining_args != 3 && remaining_args != 4) {
		usage:
		std::cout << "Invalid number of parameters.\n"
				"Usage: " << argv[0]
				<< " [-log] <memory-image> <image-address> <start-address> [<bootrom>]"
				<< std::endl;
		return -1;
	}
	std::string imageFileName = argv[first_pos_arg];
	std::string bootromFileName = "";
	if (remaining_args == 4) {
		bootromFileName = argv[first_pos_arg + 3];
	}
	bool ok;
	uintptr_t imageAddress = parse_uintptr(argv[first_pos_arg + 1],ok);
	if (!ok) {
		std::cerr << "Could not parse the image base address." << std::endl;
		return -1;
	}
	uintptr_t entryAddress = parse_uintptr(argv[first_pos_arg + 2],ok);
	if (!ok) {
		std::cerr << "Could not parse the program entry address." << std::endl;
		return -1;
	}
	// Read the image file
	std::ifstream imageFile(imageFileName.c_str(),std::ios::binary);
	if (!imageFile) {
		std::cerr << "Could not read the memory image." << std::endl;
		return -1;
	}
	auto image = read_all(imageFile);
	std::vector<unsigned char> bootrom;
	if (bootromFileName != "") {
		std::ifstream bootromFile(bootromFileName.c_str(),std::ios::binary);
		if (!bootromFile) {
			std::cerr << "Could not read the bootrom image." << std::endl;
			return -1;
		}
		bootrom = read_all(bootromFile);
	}
	// Create the simulator
	Processor *processor;
	std::vector<Device*> devices;
	if (!createSimulator(&processor, devices, verbose)) {
		return -1;
	}
	/*// Create a log file
	QFile logFile("vc4emul.log");
	if (!logFile.open(QFile::WriteOnly | QFile::Text)) {
		std::cerr << "Could not create a log file." << std::endl;
		return -1;
	}*/
	// Initialize the simulator
	Memory memory;
	Log log;
	if (logging)
		log.tofile("vc4emul.log");
	log.info("", "Initializing the simulator...");
	processor->initialize(&log, &memory);
	for (auto device : devices) {
		device->initialize(&log, &memory);
	}
	processor->setRegister("pc", entryAddress);
	log.info("", "Loading the memory image...");
	for (size_t offset = 0; offset < image.size(); offset++) {
		memory.writeByte(imageAddress + offset, image.at(offset));
	}
	if (bootromFileName != "") {
		log.info("", "Loading the bootrom image...");
		for (size_t offset = 0; offset < bootrom.size(); offset++) {
			memory.writeByte(0x60000000 + offset, bootrom.at(offset));
		}
	}
	// Start the simulation
	log.info("", "Starting the simulation...");
	try {
		while (true) {
			processor->run(1000);
		}
	} catch (const std::exception &e) {
		std::cerr << "Exception: " << e.what() << std::endl;
		const std::vector<std::string> &registers = processor->getRegisterList();
		for (auto it = registers.begin(); it != registers.end(); it++) {
			std::cerr << *it << ": " << std::hex << processor->getRegister(*it) << std::endl;
		}
		return 1;
	}
	return 0;
}
