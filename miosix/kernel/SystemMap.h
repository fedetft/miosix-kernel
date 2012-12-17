#ifndef SYSTEMMAP_H
#define	SYSTEMMAP_H

#include "kernel/sync.h"

#include <map>

namespace miosix {

class SystemMap {
public:
	SystemMap();
	virtual ~SystemMap();
	
	static SystemMap &instance();
	
	//Privata ? and friendly
	void addElfProgram(const char *name, const unsigned int *elf, unsigned int size);
	std::pair<const unsigned int*, unsigned int> getElfProgram(const char *name) const;
	
	unsigned int getElfCount() const;
	
private:
	unsigned int hashString(const char *pStr) const;
	
	SystemMap(const SystemMap& orig);
	
	typedef std::map<unsigned int, std::pair<const unsigned int*, unsigned int> > ProgramsMap;
	ProgramsMap mPrograms;
};

}

#endif	/* SYSTEMMAP_H */
