#ifndef LOO_LOO_PREDEFS_HPP
#define LOO_LOO_PREDEFS_HPP

namespace loo {
#if defined(WIN32) || defined(_WIN32) || \
    defined(__WIN32) && !defined(__CYGWIN__)
#define LOO_EXPORT __declspec(dllexport)
#else
#define LOO_EXPORT
#endif

}  // namespace loo

#endif /* LOO_LOO_PREDEFS_HPP */
