#ifndef xACAPELLA_IMACRO_SXML_XMLNODE_HA_INCLUED_
#define xACAPELLA_IMACRO_SXML_XMLNODE_HA_INCLUED_

#ifdef _MSC_VER
#pragma warning(disable: 4251)
#endif

class TiXmlNode;
class TiXmlAttribute;
class TiXmlElement;

namespace NAcapellaHttp {

	class DI_AcapellaHttp XmlNode;
	typedef NIMacro::DerivedPointer<XmlNode> PXmlNode;

	class DI_AcapellaHttp XmlAttrSet;
	typedef NIMacro::DerivedPointer<XmlAttrSet> PXmlAttrSet;

	struct MasterBlock;

	/// A MemBlock hierarchy class for encapsulating a TinyXml XML structure node. For working with TinyXML data one has to include <tinyxml/DI_tinyxml.h>.
	class DI_AcapellaHttp XmlNode: public NIMacro::MemBlock {
		typedef NIMacro::MemBlock super;
	public: // interface

		/// Creates an empty document object
		static PXmlNode Create(const NIMacro::safestring& documentname);

		/// Takes ownership and encapsulates an XML node. The parent of the node must be NULL.
		static PXmlNode Create(TiXmlNode* node=NULL);

		/// Makes a copy of the passed node and sets parent==NULL.
		static PXmlNode Create(const TiXmlNode& node);

		/// Creates additional reference to a node inside the structure held by b.
		static PXmlNode Create(const XmlNode& b, TiXmlNode* node);

		/**
		* Create a verbatim XML node. This encapsulates a raw piece of XML file content, which is output to the XML file "as is". 
		* It is up to the caller to ensure that the resulting XML file is correct.
		* *@param src The text to encapsulate.
		*/
		static PXmlNode CreateVerbatim(const char* src);

		/// Returns true if this is an XmlNull node
		bool IsNull() const {return node_==NULL;}

		/// Detaches the XmlNode object from TinuXml data structure.
		void SetNull();

		/// Returns the number of children nodes.
		int GetNodeCount() const;

		/// Returns the number of attributes.
		int GetAttrCount() const;

		/// Insert a sibling before or after this node. Returns XmlNull if this is the master node, otherwise the inserted sibling.
		PXmlNode Insert(bool before=true);

		/// Returns a refrence to the encapsulated node. 
		TiXmlNode* GetNode() const;

		/// Returns the master node for the given structure.
		TiXmlNode* GetMasterNode() const; 

		void SetAttrSet(PXmlAttrSet attrset);

		void RemoveAllAttributes();

		/// Serialize the XML tree into buffer. The previous content of buffer is kept intact, the XML stream is just appended.
		void Dump(Nbaseutil::safestring& buffer) const;

	protected: // implementation

//		/// Takes the node out of the XmlNode object; ownership is passed over.
//		TiXmlNode* TakeNode();

		MasterBlock* GetMasterBlock() const {return masterblock_;}

		/**
		* Replaces the encapsulated node. 
		* @param node New node to be associated with the XmlNode object. The node may not be associated with another XmlNode object. 
		* @param b If node's data structure has associated XmlNode objects, then a pointer to one of them must be passed here in order
		*		to attach the correct masterblock. Otherwise, NULL must be passed for creating a new masterblock.
		*/
		void SetNode(TiXmlNode* node, const XmlNode* b);

	protected: // implementation
		XmlNode(TiXmlNode* node, const XmlNode* b);
		~XmlNode();

	public: // overridden virtuals
		virtual const char*		Class() const {return "XmlNode";}
		virtual NIMacro::PMemBlock DoClone(copymode_t copy_mode) const;
		virtual bool			Conforms(const char *szClassName) const;
		virtual bool			Consistent(Nbaseutil::safestring& msg, bool CheckContent=true) const;	
		virtual bool AddConvertArg(const Nbaseutil::safestring& option, const NIMacro::DataItem& value, NIMacro::ConvertOptions& opt, Nbaseutil::ThrowHandlerFunc throwhandler=NIMacro::LogWarnings()) const;
		virtual NIMacro::SafeValue		DoVerb(const char* pszCommand, int iArg, NIMacro::SafeValue vArg[]);
		virtual bool Entertain(NIMacro::MemBlockVisitor& visitor, const Nbaseutil::safestring& name, entertainmode_t mode=enter_deep);	
		virtual void IterateChildren(NIMacro::AcaVisitor& visitor, const NIMacro::TraverseNode& node);
		virtual bool Equals(const NIMacro::MemBlock& b) const;
		virtual NIMacro::SafeValue ResolveSubItem(const Nbaseutil::safestring& subitemname, Nbaseutil::ThrowHandlerFunc throwhandler=NIMacro::ThrowIndeed) const;	
		virtual const char*	ClassDescription() const;
		virtual bool DoSetSubItem(const Nbaseutil::safestring& subitemname, const NIMacro::SafeValue& item, const Nbaseutil::safestring& fullpathname, Nbaseutil::ThrowHandlerFunc throwhandler=Nbaseutil::ThrowIndeed);
		virtual Nbaseutil::safestring DoGetDescription() const;
		virtual NIMacro::PMemBlock op(char opcode, const NIMacro::PMemBlock y, const NIMacro::op_options& options) const;
		virtual void DoSerialize(int ser_format, NIMacro::SerializeVisitor& visitor, const Nbaseutil::safestring& itemname, const NIMacro::Formatter& formatter, Nbaseutil::safestring& buffer) const;

	private: // data
		TiXmlNode* node_;
		MasterBlock* masterblock_;
	};

	inline PXmlNode XmlNull() {
		return XmlNode::Create();
	}

	/// A MemBlock hierarchy class for encapsulating single node attributes in a TinyXml XML data structure. For working with TinyXML data one has to include <tinyxml/DI_tinyxml.h>
	class DI_AcapellaHttp XmlAttrSet: public NIMacro::MemBlock {
		typedef NIMacro::MemBlock super;
	public: // interface

		/// Encapsulates the attribute set associated with elem.
		static PXmlAttrSet Create(PXmlNode elem=NULL);

		/// Return k-th attribute.
		const TiXmlAttribute* GetAttr(unsigned int k) const;

		/// Return the element owning the attribute set.
		TiXmlElement* GetElem() const;

	protected: // implementation
		XmlAttrSet(PXmlNode elem);
		~XmlAttrSet();

	protected: // overridden virtuals

		virtual const char*		Class() const {return "XmlAttrSet";}
		virtual NIMacro::PMemBlock DoClone(copymode_t copy_mode) const;
		virtual bool			Conforms(const char *szClassName) const;
		virtual bool			Consistent(Nbaseutil::safestring& msg, bool CheckContent=true) const;	
		virtual NIMacro::PMemBlock ConvertFrom(const NIMacro::PMemBlock source, const NIMacro::ConvertOptions& opt) const;
		virtual bool AddConvertArg(const Nbaseutil::safestring& option, const NIMacro::DataItem& value, NIMacro::ConvertOptions& opt, Nbaseutil::ThrowHandlerFunc throwhandler=NIMacro::LogWarnings()) const;
		virtual bool Entertain(NIMacro::MemBlockVisitor& visitor, const Nbaseutil::safestring& name, entertainmode_t mode=enter_deep);	
		virtual void IterateChildren(NIMacro::AcaVisitor& visitor, const NIMacro::TraverseNode& node);
		virtual NIMacro::SafeValue ResolveSubItem(const Nbaseutil::safestring& subitemname, Nbaseutil::ThrowHandlerFunc throwhandler=NIMacro::ThrowIndeed) const;	
		virtual const char*	ClassDescription() const;
		virtual bool DoSetSubItem(const Nbaseutil::safestring& subitemname, const NIMacro::SafeValue& item, const Nbaseutil::safestring& fullpathname, Nbaseutil::ThrowHandlerFunc throwhandler=Nbaseutil::ThrowIndeed);
		virtual Nbaseutil::safestring DoGetDescription() const;

	private: // implementation
		friend class XmlNode;
		void SetNode(XmlNode* x) {elem_=x;}

	private: // data
		PXmlNode elem_; 
	};

	// utility functions

	/// Same as TiXmlNode::ReplaceChild(pos, newnode) except maintains XmlNode internal data and checks for self-replacement.
	DI_AcapellaHttp TiXmlNode* ReplaceChild1(TiXmlNode* parent, TiXmlNode* pos, const TiXmlNode& newnode);

	
	/// Same as TiXmlNode::InsertBeforeChild(pos, newnode) except maintains XmlNode internal data.
	DI_AcapellaHttp TiXmlNode* InsertBeforeChild1(TiXmlNode* parent, TiXmlNode* pos, const TiXmlNode& newnode);

	/// Same as TiXmlNode::InsertAfterChild(pos, newnode) except maintains XmlNode internal data.
	DI_AcapellaHttp TiXmlNode* InsertAfterChild1(TiXmlNode* parent, TiXmlNode* pos, const TiXmlNode& newnode);

	/// Same as TiXmlNode::InsertEndChild(pos, newnode) except maintains XmlNode internal data.
	DI_AcapellaHttp TiXmlNode* InsertEndChild1(TiXmlNode* parent, const TiXmlNode& child);

	/**
	* Set pointers to XmlNode Acapella objects to NULL in the XML tree pointed by node. 
	* This is to be called after copying (parts of) the TinyXml structures,
	* because the copying does not flush userdata automatically.
	*/
	DI_AcapellaHttp void DetachRecursive(TiXmlNode* node);

	/** 
	* Walks the TinyXML tree pointed by node, detaches all nodes from
	* the Acapella XmlNode objects and sets these objects to XmlNull.
	* This is to be called before deleting (parts of) tinyxml structures,
	* to avoid dangling pointers in Acapella XmlNode objects.
	*/
	DI_AcapellaHttp void NullifyRecursive(TiXmlNode* node);

	/**
	* Finds the different child element names in parent. Name comparison is case sensitive.
	* @param parent Parent node whose children elements are studied.
	* @param keys Output parameter: a vector is created here storing different child element names (sorted).
	* @param keycounts Output parameter: a created vector of the same length than keys; stores the multiplicity of each child element name.
	*/
	DI_AcapellaHttp void FindKeys(TiXmlNode* parent, NIMacro::PVector& keys, NIMacro::PVector& keycounts);


	/**
	* Compose attribute/subnode table for all children elements of parent, having specified name.
	* A column will be created for each attribute of the child node and for each single-text node inside child node.
	* The last character sequence "_in_" will be replaced by "@" in column names.
	* @param parent The parent node whose child elements are studied.
	* @param name The name of the first level child elements to process.
	* @param n The number of first level child elements with specified name; the result table will have so many rows.
	* @param recursive Process the children nodes of children elements recursively. The name filtering is not applied here. The colum names will have child node names plus underscores prepended to the attribute names.
	*/
	DI_AcapellaHttp NIMacro::PTable MakeAttrTable(TiXmlNode* parent, const char* name, int n, bool recursive=true);

	/// Returns true if the parameter is syntactically valid XML tag or attribute name. All UTF-8 composite characters are considered legal.
	DI_AcapellaHttp bool IsValidXmlName(const Nbaseutil::safestring& s);

} // namespace
#endif
