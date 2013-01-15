#ifndef SYSTEMMAP_H
#define	SYSTEMMAP_H

#include "kernel/sync.h"

#include <map>
#include <string>

namespace miosix {

class SystemMap {
public:
	SystemMap();
	virtual ~SystemMap();
	
	static SystemMap &instance();
	
	//Privata ? and friendly
	void addElfProgram(const char *name, const unsigned int *elf, unsigned int size);
	void removeElfProgram(const char *name);
	std::pair<const unsigned int*, unsigned int> getElfProgram(const char *name) const;
	
	unsigned int getElfCount() const;
	
private:	
	SystemMap(const SystemMap& orig);
	
	typedef std::map<std::string, std::pair<const unsigned int*, unsigned int> > ProgramsMap;
	ProgramsMap mPrograms;
};

}

#endif	/* SYSTEMMAP_H */
