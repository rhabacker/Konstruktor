/* libLDR: Portable and easy-to-use LDraw format abstraction & I/O reference library *
 * To obtain more information about LDraw, visit http://www.ldraw.org.               *
 * Distributed in terms of the GNU Lesser General Public License v3                  *
 *                                                                                   *
 * Author: (c)2006-2008 Park "segfault" J. K. <mastermind_at_planetmono_dot_org>     */

#include <dirent.h>
#include <sys/types.h>

#include <algorithm>
#include <cstdlib>
#include <iostream>

#include "model.h"
#include "reader.h"
#include "utils.h"

#include "part_library.h"

namespace ldraw
{

#ifdef WIN32
// For win32
static const std::string predefined_path[] = {
	"\\ldraw",
	"\\Program Files\\ldraw",
	""
};
#else
static const std::string predefined_path[] = {
	"/usr/share/ldraw",
	"/usr/share/LDRAW",
	"/usr/local/share/ldraw",
	"/usr/local/share/LDRAW",
	"/opt/ldraw",
	"/opt/LDRAW",
	"~/.ldraw",
	"~/.LDRAW",
	""
};
#endif

item_refcount::item_refcount()
{
	first = 0L, second = 0;
}

item_refcount::item_refcount(model_multipart *m)
{
	set_model(m);
}

item_refcount::~item_refcount()
{
	if(first)
		delete first;
}

part_library::part_library()
{
	m_unlink_policy = parts | primitives;
	
	char *tmp = getenv("LDRAWDIR");
	
	if (!tmp) {
		int i = 0;
		while (!predefined_path[i].empty()) {
			if(init(predefined_path[i])) {
				return;
			}
			++i;
		}
	} else {
		if(init(std::string(tmp))) {
			return;
		}
	}
	
	throw exception(__func__, exception::fatal, "Couldn't find LDraw part library. Please install valid LDraw part library and make sure that \"LDRAWDIR\" environment variable is set!");
}

part_library::part_library(const std::string &path)
{
	m_unlink_policy = parts | primitives;
	
	if(!init(path))
		throw exception(__func__, exception::fatal, "Couldn't find LDraw part library.");
}

part_library::~part_library()
{
	// FIXME Apparently not working.

#if 0
	for (std::map<std::string, item_refcount*>::iterator it = m_data.begin(); it != m_data.end(); ++it)
		delete (*it).second;
#endif
}

std::string part_library::ldrawpath(path_type path_type) const
{
	switch (path_type) {
		case ldraw_path:
			return m_ldrawpath;
		case ldraw_parts_path:
			return m_ldrawpath + DIRECTORY_SEPARATOR + m_partsdir;
		case ldraw_primitives_path:
			return m_ldrawpath + DIRECTORY_SEPARATOR + m_primdir;
		default:
			return std::string();
	}
}

bool part_library::find(const std::string &name) const
{
	std::string lowercase = utils::translate_string(name);

	if (m_partlist.find(lowercase) != m_partlist.end())
		return true;
	else if (m_primlist.find(lowercase) != m_primlist.end())
		return true;

	return false;
}	

void part_library::link(model_multipart *m)
{
	link_model(m->main_model());
	
	std::map<std::string, model*> &list = m->submodel_list();
	for(std::map<std::string, model*>::iterator it = list.begin(); it != list.end(); ++it)
		link_model((*it).second);
}

bool part_library::link_element(element_ref *r)
{
	reader nil;

	if (r->get_model())
		return true;
	
	if (r->linkpoint())
		r->linkpoint()->unlink_element(r);
	
	std::string fn = utils::translate_string(r->filename());
	
	// 1. find the submodel if multipart
	if (r->parent() && r->parent()->parent())
		if (r->parent()->parent()->link_submodel_element(r))
			return true;
	
	// 2. find the model pool.
	std::map<std::string, item_refcount*>::iterator it1 = m_data.find(fn);
	if (it1 != m_data.end()) {
		(*it1).second->acquire();
		r->set_model((*it1).second->model()->main_model());
		r->resolve(this);
		return true;
	}
	
	// 3. find the primitive list
	std::map<std::string, std::string>::iterator it2 = m_primlist.find(fn);
	if (it2 != m_primlist.end()) {
		model_multipart *n = nil.load_from_file(m_ldrawpath + DIRECTORY_SEPARATOR + m_primdir + DIRECTORY_SEPARATOR + (*it2).second);
		link(n);
		r->set_model(n->main_model());
		n->main_model()->set_modeltype(model::primitive);
		m_data[(*it2).first] = new item_refcount(n);
		m_data[(*it2).first]->acquire();
		r->resolve(this);
		return true;
	}
	
	// 4. find the parts list
	std::map<std::string, std::string>::iterator it3 = m_partlist.find(fn);
	if (it3 != m_partlist.end()) {
		model_multipart *n = nil.load_from_file(m_ldrawpath + DIRECTORY_SEPARATOR + m_partsdir + DIRECTORY_SEPARATOR + (*it3).second);
		link(n);
		r->set_model(n->main_model());
		n->main_model()->set_modeltype(model::part);
		m_data[(*it3).first] = new item_refcount(n);
		m_data[(*it3).first]->acquire();
		r->resolve(this);
		return true;
	}

	if (r->parent())
		std::cerr << "File " << r->parent()->name() << ": Could not open file: " << r->filename() << std::endl;
	else
		std::cerr << "Could not open file: " << r->filename() << std::endl;
	
	return false;
}

void part_library::unlink(model_multipart *m)
{
	for (int i = 0; i < m->main_model()->size(); ++i) {
		if (m->main_model()->at(i)->get_type() == type_ref)
			unlink_element(CAST_AS_REF(m->main_model()->at(i)));
	}
	
	std::map<std::string, model*> &list = m->submodel_list();
	for (std::map<std::string, model*>::iterator it = list.begin(); it != list.end(); ++it) {
		model *tm = (*it).second;
		for (int i = 0; i < tm->size(); ++i) {
			if (tm->at(i)->get_type() == type_ref)
				unlink_element(CAST_AS_REF(tm->at(i)));
		}
	}
}

void part_library::unlink_element(element_ref *r)
{
	if (r->linkpoint() != this)
		return;
	
	std::string fn = utils::translate_string(r->filename());
	
	if (r->get_model() && (r->get_model()->modeltype() == model::submodel || r->get_model()->modeltype() == model::external_file))
		return;
	
	std::map<std::string, item_refcount*>::iterator it = m_data.find(fn);
	if (r->get_model() && it != m_data.end()) {
		(*it).second->release();
		if (!(*it).second->refcount()) {
			if (((*it).second->model()->main_model()->modeltype() == model::part && m_unlink_policy & parts) ||
				((*it).second->model()->main_model()->modeltype() == model::primitive && m_unlink_policy & primitives)) {
				delete (*it).second;
				m_data.erase(it);
			}
		}
	}
	
	r->set_model(0L);
	r->resolve(0L);
}

bool part_library::init(const std::string &path)
{
	// TODO recursive subdirectory handling
	DIR *de;
	struct dirent *ep;
	std::string dn1, dn2;
	std::string pdir, partsdir;
	
	// 1. find subdirectories
	if ((de = opendir(path.c_str())) == 0L)
		return false;
	
	while ((ep = readdir(de))) {
		dn1 = ep->d_name;
		dn2 = utils::translate_string(dn1);
		
		if (dn2 == "p") pdir = dn1;
		else if (dn2 == "parts") partsdir = dn1;
	}
	closedir(de);
	
	if (pdir.empty() || partsdir.empty()) {
		std::cerr << "[libLDR] No p/ or parts/ found." << std::endl;
		return false;
	}
	
	m_ldrawpath = path;
	m_primdir = pdir;
	m_partsdir = partsdir;
	
	// 2. look into p/ directory
	if ((de = opendir((m_ldrawpath + DIRECTORY_SEPARATOR + pdir).c_str())) == 0L) {
		std::cerr << "[libLDR] Couldn't open p/." << std::endl;
		return false;
	}
	while ((ep = readdir(de))) {
		dn1 = ep->d_name;
		dn2 = utils::translate_string(dn1);
		
		if (dn2.length() > 4 && dn2.substr(dn1.length()-4, 4) == ".dat")
			m_primlist[dn2] = dn1;
	}
	closedir(de);
	
	// 3. look into p/48 directory
	if ((de = opendir((m_ldrawpath + DIRECTORY_SEPARATOR + pdir + DIRECTORY_SEPARATOR + "48").c_str())) == 0L)
		std::cerr << "[libLDR] Couldn't open p/48/." << std::endl;
	else {
		while((ep = readdir(de))) {
			dn1 = std::string("48") + DIRECTORY_SEPARATOR + ep->d_name;
			dn2 = utils::translate_string(dn1);
			
			if (dn2.length() > 4 && dn2.substr(dn2.length()-4, 4) == ".dat")
				m_primlist[dn2] = dn1;
		}
		closedir(de);
	}
	
	// 4. look into parts/ directory
	if ((de = opendir((m_ldrawpath + DIRECTORY_SEPARATOR + m_partsdir).c_str())) == 0L) {
		std::cerr << "[libLDR] Couldn't open parts/." << std::endl;
		return false;
	}
	while((ep = readdir(de))) {
		dn1 = ep->d_name;
		dn2 = utils::translate_string(dn1);
		
		if (dn2.length() > 4 && dn2.substr(dn2.length() - 4, 4) == ".dat")
			m_partlist[dn2] = dn1;
	}
	closedir(de);
	
	// 5. look into parts/s directory
	std::string s;
	de = opendir((m_ldrawpath + DIRECTORY_SEPARATOR + m_partsdir + DIRECTORY_SEPARATOR + "s").c_str());
	s = "s";
	
	if(!de) {
		de = opendir((m_ldrawpath + DIRECTORY_SEPARATOR + m_partsdir + DIRECTORY_SEPARATOR + "S").c_str());
		s = "S";
	}
	
	if(!de)
		std::cerr << "[libLDR] Couldn't open parts/s/." << std::endl;
	else {
		while((ep = readdir(de))) {
			dn1 = s + DIRECTORY_SEPARATOR + ep->d_name;
			dn2 = utils::translate_string(dn1);
			
			if (dn2.length() > 4 && dn2.substr(dn2.length() - 4, 4) == ".dat")
				m_partlist[dn2] = dn1;
		}
		closedir(de);
	}
	
	return true;
}

void part_library::link_model(model *m)
{
	for (int i = 0; i < m->size(); ++i) {
		if (m->at(i)->get_type() == type_ref) {
			element_ref *r = CAST_AS_REF(m->at(i));
			if (link_element(r)) {
				if(r->get_model())
					link_model(r->get_model());
			}
		}
	}
}

}
