/*
	OpenFirmware Tree.h
	Copyright © 1998 by Terry Greeniaus

	This program is free software; you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation; either version 2 of the License, or
	(at your option) any later version.
	
	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.
	
	You should have received a copy of the GNU General Public License
	along with this program; if not, write to the Free Software
	Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
*/
/*
	Other sources			Project				Author			Notes
	===========			======				=====			====
	none
	
	Version History
	============
	Terry Greeniaus	-	Tuesday, 21 July 1998	-	Original creation of file
*/
#ifndef __OPENFIRMWARE_TREE__
#define __OPENFIRMWARE_TREE__

enum
{
	unusedNodeType	=	0,	// Denotes an unused node in the tree
	cpuNodeType		=	1,	// A CPU node
	memoryNodeType	=	2,	// A memory range node
	cacheNodeType		=	3,	// A cache node
	deviceNodeType	=	4,	// A device node
	emptyNodeType		=	5,	// An empty node (but can have children nodes that aren't empty)
	pciNodeType		=	6	// A PCI bridge device
};

typedef struct _OpenFirmwareCPUNode
{
	// This is a CPU node, describing various CPU characteristics
	UInt32	_cpuVersion;		// The PVR of this CPU
	UInt32	_clockFrequency;	// The speed of this CPU in Hz
	UInt32	_busFrequency;		// The speed of this CPU's bus
	UInt32	_dCacheSize;		// The size of the data cache
	UInt32	_iCacheSize;		// The size of the instruction cache
}_OpenFirmwareCPUNode;

typedef struct _OpenFirmwareMemoryNode
{
	// This is a memory node, describing a range of physical memory
	UInt32	_baseAddr;		// The base address of this memory range
	UInt32	_size;			// The physical size of this memory range
}_OpenFirmwareMemoryNode;

typedef struct _OpenFirmwareCacheNode
{
	// This is a cache node, usually the motherboard cache
	UInt32	_iCacheSize;		// The size of the instruction cache
	UInt32	_dCacheSize;		// The size of the data cache
	UInt32	_iCacheSets;		// The number of instruction cache sets
	UInt32	_dCacheSets;		// The number of data cache sets
	Boolean	_unified;			// True if this is a unifed cache (data/instruction cached together).
}_OpenFirmwareCacheNode;

typedef struct _OpenFirmwareDeviceNode
{
	// This is a device node
	UInt32	_address[4];		// OpenFirmware device addresses (-1 if unused)
	UInt32	_interrupts[4];		// OpenFirmware interrupt numbers (-1 if unused)
}_OpenFirmwareDeviceNode;

#define MAX_PCI_IRQ_COUNT	32	// The maximum number of IRQs to be stored in _OpenFirmwarePCINode.  Bump it up if you need to

typedef struct _OpenFirmwarePCINode
{
	// This is a PCI bridge node
	UInt32	_reg[2];							// "reg" property
	UInt32	_busRange[2];						// "bus-range" property
	UInt8	_devFN[MAX_PCI_IRQ_COUNT];		// interrupt-map devFN stuff
	UInt8	_irq[MAX_PCI_IRQ_COUNT];			// interrupt-map irq stuff
	UInt8	_numIRQs;
}_OpenFirmwarePCINode;

typedef struct _OpenFirmwareNode
{
	// This is a 128-byte structure describing the node
	UInt32	_nodeType;	// See list above
	UInt32	_parentNode;	// The parent node of this node, -1 if the top node
	UInt32	_prevSibling;	// The previous sibling of this node, -1 if the first node
	UInt32	_nextSibling;	// The next sibling of this node, -1 if the last node
	UInt32	_childNode;	// The first child of this node, -1 if none.  To get all children of this node, get the child node and then walk the sibling list
	ASCII8	_name[47];	// This node's name
	ASCII8	_devType[29];	// This node's device-type
	union
	{
		_OpenFirmwareCPUNode		cpuNode;
		_OpenFirmwareMemoryNode	memoryNode;
		_OpenFirmwareCacheNode		cacheNode;
		_OpenFirmwareDeviceNode	deviceNode;
		_OpenFirmwarePCINode		pciNode;
	};
}_OpenFirmwareNode;

class OpenFirmwareTree
{
	UInt32					_numNodes;		// The number of nodes in the tree
	_OpenFirmwareNode*		_nodes;			// The nodes, all flattened into the tree
	class OpenFirmwareNode*	*	_nodeObjects;		// The C++ objects representing each node
	
	_OpenFirmwareNode*		newNode(OpenFirmwareNode* nodeObject);	// Gets a free node from the tree, assigning it to nodeObject
	UInt32					getNodeNum(OpenFirmwareNode* node);		// Gets the number of an OpenFirmwareNode
public:
	OpenFirmwareTree(UInt32 numNodes);	// Creates an tree with a single node (0)
	OpenFirmwareTree(Int8* flattenedTree);		// Creates an OpenFirmwareTree from the flattened tree
	
	OpenFirmwareNode*	getNode(UInt32 n);		// Gets node n from the tree.  Node 0 is created by default and is the top of the tree
	OpenFirmwareNode*	getNode(ConstASCII8Str name);	// Gets the node with the given name from the tree
	OpenFirmwareNode*	getNextNode(ConstASCII8Str name,UInt32 prevNode);	// Gets the next node with the given name, from prevNode
	OpenFirmwareNode*	getNextNode(ConstASCII8Str name,OpenFirmwareNode* prevNode);	// Gets the next node with the given name, from prevNode
	
	OpenFirmwareNode*	getNodeByDevType(ConstASCII8Str devType);	// Gets the node with the given device type from the tree
	OpenFirmwareNode*	getNextNodeByDevType(ConstASCII8Str devType,OpenFirmwareNode* prevNode);	// Gets the next node with the given device type, from prevNode
	
	UInt32			getFlattenedTreeLength();		// Returns the length of the flattened tree
	void				flattenTree(Int8* buffer);	// Flattens the tree into the buffer
	
	friend class OpenFirmwareNode;
};

class OpenFirmwareNode
{
protected:
	_OpenFirmwareNode*	_myNode;
	OpenFirmwareTree*		_myTree;
	
	OpenFirmwareNode(_OpenFirmwareNode* myNode,OpenFirmwareTree* myTree);
	OpenFirmwareNode(UInt32 nodeType,OpenFirmwareNode* parentNode,ConstASCII8Str name,ConstASCII8Str devType);
	virtual ~OpenFirmwareNode();
public:
	UInt32			nodeType();
	OpenFirmwareNode*	parentNode();
	OpenFirmwareNode*	nextSibling();
	OpenFirmwareNode*	prevSibling();
	OpenFirmwareNode*	firstChild();
	ConstASCII8Str		name();
	ConstASCII8Str		devType();
};

class OpenFirmwareCPUNode	:	public OpenFirmwareNode
{
	OpenFirmwareCPUNode(_OpenFirmwareNode* myNode,OpenFirmwareTree* myTree);
public:
	OpenFirmwareCPUNode(OpenFirmwareNode* parentNode,ConstASCII8Str name,UInt32 cpuVersion,UInt32 clockFrequency,
		UInt32 busFrequency,UInt32 dCacheSize,UInt32 iCacheSize);
	
	UInt32	cpuVersion();
	UInt32	clockFrequency();
	UInt32	busFrequency();
	UInt32	dCacheSize();
	UInt32	iCacheSize();
	
	friend class OpenFirmwareTree;
};

class OpenFirmwareMemoryNode	:	public OpenFirmwareNode
{
	OpenFirmwareMemoryNode(_OpenFirmwareNode* myNode,OpenFirmwareTree* myTree);
public:
	OpenFirmwareMemoryNode(OpenFirmwareNode* parentNode,ConstASCII8Str name,UInt32 baseAddr,UInt32 size);
	
	UInt32	baseAddr();
	UInt32	size();
	
	friend class OpenFirmwareTree;
};

class OpenFirmwareCacheNode	:	public OpenFirmwareNode
{
	OpenFirmwareCacheNode(_OpenFirmwareNode* myNode,OpenFirmwareTree* myTree);
public:
	OpenFirmwareCacheNode(OpenFirmwareNode* parentNode,ConstASCII8Str name,UInt32 iCacheSize,
		UInt32 dCacheSize,UInt32 iCacheSets,UInt32 dCacheSets,Boolean unified);
	
	UInt32	iCacheSize();
	UInt32	dCacheSize();
	UInt32	iCacheSets();
	UInt32	dCacheSets();
	Boolean	unified();
	
	friend class OpenFirmwareTree;
};

class OpenFirmwareDeviceNode	:	public OpenFirmwareNode
{
	OpenFirmwareDeviceNode(_OpenFirmwareNode* myNode,OpenFirmwareTree* myTree);
public:
	OpenFirmwareDeviceNode(OpenFirmwareNode* parentNode,ConstASCII8Str name,ConstASCII8Str devType,UInt32 address0,UInt32 address1,UInt32 address2,
		UInt32 address3,UInt32 interrupt0,UInt32 interrupt1,UInt32 interrupt2,UInt32 interrupt3);
	
	UInt32	address(UInt32 n);
	UInt32	interrupt(UInt32 n);
	
	friend class OpenFirmwareTree;
};

class OpenFirmwarePCINode	:	public OpenFirmwareNode
{
	OpenFirmwarePCINode(_OpenFirmwareNode* myNode,OpenFirmwareTree* myTree);
public:
	OpenFirmwarePCINode(OpenFirmwareNode* parentNode,ConstASCII8Str name,UInt32 reg0,UInt32 reg1,UInt32 busRange0,UInt32 busRange1,UInt8* devFNList,UInt8* devIRQList,UInt8 numIRQs);
	
	UInt32	reg(UInt32 n);
	UInt32	busRange(UInt32 n);
	UInt32	irq(UInt8 bus,UInt8 devFN);	// Returns -1 if no IRQ found
};

class OpenFirmwareEmptyNode	:	public OpenFirmwareNode
{
	OpenFirmwareEmptyNode(_OpenFirmwareNode* myNode,OpenFirmwareTree* myTree);
public:
	OpenFirmwareEmptyNode(OpenFirmwareNode* parentNode,ConstASCII8Str name,ConstASCII8Str devType);
	
	friend class OpenFirmwareTree;
};

#endif /* __OPENFIRMWARE_TREE__ */
