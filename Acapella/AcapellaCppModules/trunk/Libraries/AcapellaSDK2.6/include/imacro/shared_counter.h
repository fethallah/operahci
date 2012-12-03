#ifndef x_PKI_CTG_ACAPELLA_IMACRO_SHARED_COUNTER_H_INCLUDED
#define x_PKI_CTG_ACAPELLA_IMACRO_SHARED_COUNTER_H_INCLUDED

#include <boost/cstdint.hpp>

namespace NIMacro {

	namespace detail {
		
		class DI_IMacro SharedCounterImpl {
		public:
			typedef boost::uint32_t key_t;
		protected:
			static void SetResourceLimit(key_t key, int limit, size_t tag_len);
			static int GetResourceLimit(key_t key);
			static int GetResourceUsage(key_t key);
			static bool Acquire(key_t key, const void* tag, size_t tag_len);
			static void Release(key_t key, const void* tag, size_t tag_len);
		};
	} // namespace detail


	/**
	* A singleton class, implements system-wide resource counters common to all processes.
	* @param key_t Different resource types are identified by different keys. This type must be a POD and may not cointain any pointers or references.
	* @param tag_t Identifies a consumed piece of resource. For each resource one can consume up to some limit of different tags. This type must be a POD and may not cointain any pointers or references.
	*/
	template<typename tag_t>
	class SharedCounter: private detail::SharedCounterImpl {
		typedef detail::SharedCounterImpl super;
	public: // interface
		static void SetResourceLimit(key_t key, int limit) {
			super::SetResourceLimit(key, limit, sizeof(tag_t));
		}
		static int GetResourceLimit(key_t key) {
			return super::GetResourceLimit(key);
		}
		static int GetResourceUsage(key_t key) {
			return super::GetResourceUsage(key);
		}
		/**
		* Consume a resource. For each Consume() there must occur a corresponding Release(). Release() is also automatic in case of process termination.
		* Only different tag values are counted when calculating the limit. 
		* @return True in the case of success, false if the limit was exceeded. In the latter case no change of state happens.
		*/
		static bool Acquire(key_t key, tag_t tag) {
			return super::Acquire(key, &tag, sizeof(tag_t));
		}
		/**
		* Release a resource. The resource must be released in the same process which called Consume(). For each Consume()
		* there must occur a corresponding Release(), unless the process is terminated, in which case its consumed resources 
		* are automatically released.
		*/
		static void Release(key_t key, tag_t tag) {
			super::Release(key, &tag, sizeof(tag_t));
		}
	};

} // namespace
#endif
