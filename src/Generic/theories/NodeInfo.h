// Copyright 2008 by BBN Technologies Corp.
// All Rights Reserved.

#ifndef NODE_INFO_H
#define NODE_INFO_H

/** This module defines NodeInfo, which provides static methods
 * that can be used to check whether a SynNode belongs to a given 
 * category.  Examples include isOfNPKind() and isNominalPremod().
 * These static methods are given language-specific definitions,
 * by delegating to a singleton instance of a langauge-specific
 * subclass of NodeInfoInstance. */

class SynNode;

#include <boost/shared_ptr.hpp>
#include <boost/noncopyable.hpp>

/** A collection of static methods that can be used to check whether a
 * given SynNode belongs to some category.  These methods delgate to a
 * private language-specific singleton, which in turn delegates to
 * static methods in a language-specific NodeInfo class, such as
 * EnglishNodeInfo or ArabicNodeInfo.
 *
 * These language-specific classes (such as EnglishNodeInfo and
 * ArabicNodeInfo) should *not* be subclasses of NodeInfo;
 * in particular, if a language-specific NodeInfo class was
 * subclassed from NodeInfo, and neglected to implement some
 * static method, then any call to that method would result in an
 * infinite delegation loop.
 *
 * The 'NodeInfo::setImplementation()' method can be used to set the
 * language-specific NodeInfo class that is used to implement
 * the static methods.
 *
 * Related language-specific classes, such as EnglishNodeInfo,
 * may define additional static methods, as well as static constant
 * Symbols.  These should be accessed directly from the class (rather
 * than via the NodeInfo superclass).
 *
 * The NodeInfo class itself should *not* include any static
 * constant Symbols, since these are by definition language-specific.
 */
class NodeInfo {
public:
	/** Set the language-specific NodeInfo instance that should be
	 * used to implement the static methods in this class.  E.g.:
	 * NodeInfo::setImplementation<EnglishNodeInfo>(); */
	template<typename LanguageNodeInfo>
	static void setImplementation() {
		getInstance() = boost::shared_ptr<NodeInfoInstance>
			(_new(NodeInfoInstanceFor<LanguageNodeInfo>));
	}

	static bool isReferenceCandidate(const SynNode *node) {
		return getInstance()->isReferenceCandidate(node); }
	static bool isOfNPKind(const SynNode *node) {
		return getInstance()->isOfNPKind(node); }
	static bool isOfHobbsNPKind(const SynNode *node) {
		return getInstance()->isOfHobbsNPKind(node); }
	static bool isOfHobbsSKind(const SynNode *node) {
		return getInstance()->isOfHobbsSKind(node); }
	static bool canBeNPHeadPreterm(const SynNode *node) {
		return getInstance()->canBeNPHeadPreterm(node); }
	static bool isNominalPremod(const SynNode *node) {
		return getInstance()->isNominalPremod(node); }

private:
	NodeInfo(); // private constructor: this class may not be instantiated
	
	/** Abstract base class for the singletons that implement
	 * language-specific node info methods.  The templated class
	 * NodeInfoInstanceFor<T>, defined below, is used to generate
	 * language-specific implementations of this class. */
	class NodeInfoInstance: private boost::noncopyable {
	public:
		virtual ~NodeInfoInstance() {}
		// Define a single abstract method corresponding to each method in
		// the NodeInfo class.
		virtual bool isReferenceCandidate(const SynNode *node) const = 0;
		virtual bool isOfNPKind(const SynNode *node) const = 0;
		virtual bool isOfHobbsNPKind(const SynNode *node) const = 0;
		virtual bool isOfHobbsSKind(const SynNode *node) const = 0;
		virtual bool canBeNPHeadPreterm(const SynNode *node) const = 0;
		virtual bool isNominalPremod(const SynNode *node) const = 0;
	};

	/** Return a pointer to the singleton NodeInfoInstance used
	 * by the static methods for delegating to language-specific
	 * implementations. */
	static boost::shared_ptr<NodeInfoInstance> &getInstance();

	/** Templated implementation of NodeInfoInstance, based on a
	 * given class.  That class must define a static method
	 * corresponding to each method defined by NodeInfo. */
	template<typename T>
	class NodeInfoInstanceFor: public NodeInfoInstance {
	public:
		// Define a method corresponding to each method in the
		// NodeInfo class, that delgates to T.
		bool isReferenceCandidate(const SynNode *node) const { 
			return T::isReferenceCandidate(node); }
		bool isOfNPKind(const SynNode *node) const {
			return T::isOfNPKind(node); }
		bool isOfHobbsNPKind(const SynNode *node) const {
			return T::isOfHobbsNPKind(node); }
		bool isOfHobbsSKind(const SynNode *node) const {
			return T::isOfHobbsSKind(node); }
		bool canBeNPHeadPreterm(const SynNode *node) const {
			return T::canBeNPHeadPreterm(node); }
		bool isNominalPremod(const SynNode *node) const {
			return T::isNominalPremod(node); }
	};
};

#endif
