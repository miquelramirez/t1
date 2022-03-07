#ifndef __LISTSET_H__
#define __LISTSET_H__

namespace NFF
{

  class ListSet : public std::list<unsigned> {

  public:

    void insert( ListSet &other ) {
          
      ListSet::iterator it1, it2;
    
      it1 = begin();
      it2 = other.begin();
    
      while((it1 != end()) && (it2 != other.end())) {
	// do sorted insertions
	if(*it1 == *it2) {
	  it1++;
	  it2++;
	}
	else if(*it1 > *it2) {
	  std::list<unsigned>::insert(it1, *it2);
	  it2++;
	}
	else {
	  it1++;
	}
      }
      // insert at end
      while(it2 != other.end()) {
	std::list<unsigned>::insert(it1, *it2);
	it2++;
      }
    }

    void subtract( ListSet &other ) {
      ListSet::iterator it1, it2, tmp;

      it1 = begin();
      it2 = other.begin();

      while((it1 != end()) && (it2 != other.end())) {
	if(*it1 == *it2) {
	  tmp = it1;
	  it1++;
	  erase(tmp);
	  it2++;
	}
	else if(*it1 > *it2) {
	  it2++;
	}
	else {
	  it1++;
	}
      } 
    }

    void intersect( ListSet &other ) {

      ListSet::iterator it1, it2, tmp;

      it1 = begin();
      it2 = other.begin();
      
      while((it1 != end()) && (it2 != other.end())) {

	if(*it1 == *it2) {
	  it1++;
	  it2++;
	}
	else if(*it1 < *it2) {
	  tmp = it1;
	  it1++;
	  erase(tmp);
	}
	else {
	  it2++;
	}
      }

      erase(it1, end());
    }

    bool insert( unsigned a ) {
      ListSet::iterator it1;
      it1 = begin();
      while( (it1 != end() && (*it1 < a) )) it1++;
      if(a != *it1) {
	std::list<unsigned>::insert(it1, a);
	return true;
      }
      return false;
    }

    iterator find( unsigned a ) {
      ListSet::iterator it;
      it = begin();
      while((it != end()) && (*it < a)) it++;
      if(it == end()) return end();
      if(*it == a) return it;
      return end();
    }

    void printIndices( std::ostream& os ) {
      std::list<unsigned>::iterator it;
      std::cout << "{";
      for(it = begin(); it != end();) {
	std::cout << *it;
	if(++it != end())
	  std::cout << ", ";
      }
      std::cout << "}";
    }

    void printIndices() { printIndices(std::cout); }
  };

}

#endif
