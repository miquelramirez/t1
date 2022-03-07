#ifndef __HASH_TABLE__
#define __HASH_TABLE__

#include <vector>
#include <list>
#include <algorithm>
#include "jenkins_12bit.hxx"
#include "nff_planner_stats.hxx"

class Hash_Key
{
public:

	Hash_Key()
		: m_code(0)
	{
	}

	Hash_Key( const Hash_Key& other )
	{
		m_code = other.m_code;
	}

	const Hash_Key& operator=( const Hash_Key& other )
	{
		m_code = other.m_code;
		return *this;
	}

	~Hash_Key()
	{
	}
	
	void add( unsigned k );
	void add( std::vector<unsigned>& k );

	operator unsigned() const
	{
		return m_code;
	}

protected:

	unsigned	m_code;
};

inline void Hash_Key::add( unsigned k )
{
	m_code = jenkins_hash( (ub1*)&k, sizeof(unsigned), m_code );
}

inline void Hash_Key::add( std::vector<unsigned>& k )
{
	if ( k.empty() )
	{
		unsigned k2 = 0;
		m_code = jenkins_hash( (ub1*)&k2, sizeof(unsigned), m_code );
		return;
	}

	std::sort( k.begin(), k.end() );
	m_code = jenkins_hash( (ub1*)(&k[0]), sizeof(unsigned), m_code );
	for ( unsigned i = 1; i < k.size(); i++ )
	{
		m_code = jenkins_hash( (ub1*)(&k[i]), sizeof(unsigned), m_code );
	}	
	
}

template <typename T>
class Hash_Table
{
public:
	typedef T								Object;
	typedef std::list< std::pair< unsigned, Object* > > 			Node_List;
	typedef std::vector< Node_List > 					Table;

	Hash_Table( unsigned hash_sz );
	~Hash_Table();
	
	void clear();

	Object* get_element( Object* other )
	{
		assert( m_buckets.size() > 0 );
		unsigned h = other->hash();
		unsigned address = h & ( m_buckets.size()-1 );
#ifdef DEBUG
		std::cout << "Hashtable::get_element() : has to check " << m_buckets[address].size();
		std::cout << " nodes for equality (total " << n_elems << " in hashtable)" << std::endl;
			
#endif
		for ( typename Node_List::iterator i = m_buckets[address].begin();
			i != m_buckets[address].end(); i++ )
			if ( i->first == h )
			{
			       	if( *other == *(i->second) )  
				{
				       	return i->second;
				}
			}
		return NULL;
	}	

	void add_element( Object* obj )
	{
#ifdef DEBUG
		assert( get_element( obj ) == NULL );
#endif
//                NFF::Planner_Stats&	stats = NFF::Planner_Stats::instance();

		unsigned h = obj->hash();
		unsigned address = h & ( m_buckets.size() - 1);
		m_buckets[address].push_back( std::make_pair( h, obj ) );
		n_elems++;
//                stats.notify_hashtable_node(n_elems);
	}

	void remove_element( Object* obj )
	{
		unsigned h = obj->hash();
		unsigned address = h & ( m_buckets.size() - 1);
#ifdef DEBUG
		std::cout << "Hashtable::remove_element() : has to check " << m_buckets[address].size();
		std::cout << " nodes for equality (total " << n_elems << " in hashtable)" << std::endl;
			
#endif		
		typename  Node_List::iterator p = m_buckets[address].end();
		for ( typename Node_List::iterator i = m_buckets[address].begin();
			i != m_buckets[address].end(); i++ )
		{
			if ( i->first == h && ( *obj == *(i->second)) ) // element found
			{
			  // check this with miquel, should we be deleting things here?
			  //				delete i->second;
				p = i;
				break;
			}
		}	
		if ( p != m_buckets[address].end() )
		{
			m_buckets[address].erase( p );	
			n_elems--;
		}
	}
	
protected:
	Table 		m_buckets;
	unsigned	n_elems;
};

// Inlined methods
template <typename T>
Hash_Table<T>::Hash_Table( unsigned hash_sz )
	: n_elems(0)
{
	m_buckets.resize(hash_sz);
}

template <typename T>
Hash_Table<T>::~Hash_Table()
{
	for ( unsigned i = 0; i < m_buckets.size(); i++ )
	{
		for ( typename Node_List::iterator it = m_buckets[i].begin();
			it != m_buckets[i].end(); it++ )
			delete it->second;
		m_buckets[i].clear();	
	}
	m_buckets.clear();
}

template <typename T>
void Hash_Table<T>::clear()
{
	for ( unsigned i = 0; i < m_buckets.size(); i++ )
	{
		for ( typename Node_List::iterator it = m_buckets[i].begin();
			it != m_buckets[i].end(); it++ )
			delete it->second;
		m_buckets[i].clear();	
	}
	n_elems = 0;
}

#endif
