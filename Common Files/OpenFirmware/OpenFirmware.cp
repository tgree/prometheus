/*
	OpenFirmware.cp
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
	Terry Greeniaus	-	Tuesday, 26 Oct 1999	-	Added support for saving "device_type" property
*/
#include "Kernel Types.h"
#include "ANSI.h"
#include "OpenFirmware.h"

OpenFirmwareTree::OpenFirmwareTree(UInt32 numNodes)
{
	_numNodes = numNodes;
	_nodes = new _OpenFirmwareNode[numNodes];
	_nodeObjects = new OpenFirmwareNode*[numNodes];
	
	_nodes[0]._nodeType = emptyNodeType;
	_nodes[0]._parentNode = -1;
	_nodes[0]._prevSibling = -1;
	_nodes[0]._nextSibling = -1;
	_nodes[0]._childNode = -1;
	strcpy(_nodes[0]._name,"device-tree");
	
	_nodeObjects[0] = new OpenFirmwareEmptyNode(&_nodes[0],this);
	
	for(Int32 i=1;i<_numNodes;i++)
		_nodes[i]._nodeType = unusedNodeType;
}

OpenFirmwareTree::OpenFirmwareTree(Int8* tree)
{
	_numNodes = 1;
	_nodes = (_OpenFirmwareNode*)tree;
	
	// Count the nodes in the tree
	for(UInt32 currNode = 0;currNode < _numNodes;currNode++)
	{
		if(_nodes[currNode]._parentNode >= _numNodes && _nodes[currNode]._parentNode != -1)
			_numNodes = _nodes[currNode]._parentNode + 1;
		if(_nodes[currNode]._prevSibling >= _numNodes && _nodes[currNode]._prevSibling != -1)
			_numNodes = _nodes[currNode]._prevSibling + 1;
		if(_nodes[currNode]._nextSibling >= _numNodes && _nodes[currNode]._nextSibling != -1)
			_numNodes = _nodes[currNode]._nextSibling + 1;
		if(_nodes[currNode]._childNode >= _numNodes && _nodes[currNode]._childNode != -1)
			_numNodes = _nodes[currNode]._childNode + 1;
	}
	
	_nodeObjects = new OpenFirmwareNode*[_numNodes];
	for(Int32 i=0;i<_numNodes;i++)
	{
		switch(_nodes[i]._nodeType)
		{
			case cpuNodeType:		_nodeObjects[i] = new OpenFirmwareCPUNode(&_nodes[i],this);	break;
			case memoryNodeType:	_nodeObjects[i] = new OpenFirmwareMemoryNode(&_nodes[i],this);	break;
			case cacheNodeType:		_nodeObjects[i] = new OpenFirmwareCacheNode(&_nodes[i],this);	break;
			case deviceNodeType:	_nodeObjects[i] = new OpenFirmwareDeviceNode(&_nodes[i],this);	break;
			default:				_nodeObjects[i] = new OpenFirmwareEmptyNode(&_nodes[i],this);	break;
		}
	}
}

_OpenFirmwareNode* OpenFirmwareTree::newNode(OpenFirmwareNode* nodeObject)
{
	for(Int32 i=0;i<_numNodes;i++)
	{
		if(_nodes[i]._nodeType == unusedNodeType)
		{
			_nodes[i]._nodeType = emptyNodeType;
			_nodeObjects[i] = nodeObject;
			return &_nodes[i];
		}
	}
	
	return nil;
}

UInt32 OpenFirmwareTree::getNodeNum(OpenFirmwareNode* node)
{
	for(Int32 i = 0;i<_numNodes;i++)
	{
		if(_nodeObjects[i] == node)
			return i;
	}
	return -1;
}

OpenFirmwareNode* OpenFirmwareTree::getNode(UInt32 n)
{
	return (n < _numNodes ? _nodeObjects[n] : nil);
}

OpenFirmwareNode* OpenFirmwareTree::getNode(ConstASCII8Str name)
{
	for(Int32 i=0;i<_numNodes;i++)
	{
		if(!strcmp(name,_nodes[i]._name))
			return _nodeObjects[i];
	}
	return nil;
}

OpenFirmwareNode* OpenFirmwareTree::getNextNode(ConstASCII8Str name,UInt32 prevNode)
{
	for(Int32 i=prevNode+1;i<_numNodes;i++)
	{
		if(!strcmp(name,_nodes[i]._name))
			return _nodeObjects[i];
	}
	return nil;
}

OpenFirmwareNode* OpenFirmwareTree::getNextNode(ConstASCII8Str name,OpenFirmwareNode* prevNode)
{
	for(Int32 i = getNodeNum(prevNode) + 1;i<_numNodes;i++)
	{
		if(!strcmp(name,_nodes[i]._name))
			return _nodeObjects[i];
	}
	return nil;
}

OpenFirmwareNode* OpenFirmwareTree::getNodeByDevType(ConstASCII8Str devType)
{
	for(Int32 i=0;i<_numNodes;i++)
	{
		if(!strcmp(devType,_nodes[i]._devType))
			return _nodeObjects[i];
	}
	return nil;
}

OpenFirmwareNode* OpenFirmwareTree::getNextNodeByDevType(ConstASCII8Str devType,OpenFirmwareNode* prevNode)
{
	for(Int32 i = getNodeNum(prevNode) + 1;i<_numNodes;i++)
	{
		if(!strcmp(devType,_nodes[i]._devType))
			return _nodeObjects[i];
	}
	return nil;
}

UInt32 OpenFirmwareTree::getFlattenedTreeLength()
{
	return (sizeof(_OpenFirmwareNode)*_numNodes);
}

void OpenFirmwareTree::flattenTree(Int8* buffer)
{
	memcpy(buffer,_nodes,getFlattenedTreeLength());
}

OpenFirmwareNode::OpenFirmwareNode(_OpenFirmwareNode* myNode,OpenFirmwareTree* myTree)
{
	_myTree = myTree;
	_myNode = myNode;
}

OpenFirmwareNode::OpenFirmwareNode(UInt32 nodeType,OpenFirmwareNode* parentNode,ConstASCII8Str name,ConstASCII8Str devType)
{
	_myTree = parentNode->_myTree;
	_myNode = _myTree->newNode(this);
	_myNode->_nodeType = nodeType;
	_myNode->_parentNode = _myTree->getNodeNum(parentNode);
	if(parentNode->_myNode->_childNode != -1)
	{
		UInt32 siblingNode = parentNode->_myNode->_childNode;
		OpenFirmwareNode*	sibling = _myTree->getNode(siblingNode);
		for(;;)
		{
			OpenFirmwareNode*	nextSibling = sibling->nextSibling();
			if(nextSibling)
				sibling = nextSibling;
			else
				break;
		}
		sibling->_myNode->_nextSibling = _myTree->getNodeNum(this);
		_myNode->_prevSibling = _myTree->getNodeNum(sibling);
	}
	else
	{
		parentNode->_myNode->_childNode = _myTree->getNodeNum(this);
		_myNode->_prevSibling = -1;
	}
	_myNode->_nextSibling = -1;
	_myNode->_childNode = -1;
	strcpy(_myNode->_name,name);
	strcpy(_myNode->_devType,devType);
}

OpenFirmwareNode::~OpenFirmwareNode()
{
}

UInt32 OpenFirmwareNode::nodeType()
{
	return _myNode->_nodeType;
}

OpenFirmwareNode* OpenFirmwareNode::parentNode()
{
	return (_myNode->_parentNode != -1 ? _myTree->getNode(_myNode->_parentNode) : nil);
}

OpenFirmwareNode* OpenFirmwareNode::nextSibling()
{
	return (_myNode->_nextSibling != -1 ? _myTree->getNode(_myNode->_nextSibling) : nil);
}

OpenFirmwareNode* OpenFirmwareNode::prevSibling()
{
	return (_myNode->_prevSibling != -1 ? _myTree->getNode(_myNode->_prevSibling) : nil);
}

OpenFirmwareNode* OpenFirmwareNode::firstChild()
{
	return (_myNode->_childNode != -1 ? _myTree->getNode(_myNode->_childNode) : nil);
}

ConstASCII8Str OpenFirmwareNode::name()
{
	return _myNode->_name;
}

ConstASCII8Str OpenFirmwareNode::devType()
{
	return _myNode->_devType;
}

OpenFirmwareCPUNode::OpenFirmwareCPUNode(_OpenFirmwareNode* myNode,OpenFirmwareTree* myTree):
	OpenFirmwareNode(myNode,myTree)
{
}

OpenFirmwareCPUNode::OpenFirmwareCPUNode(OpenFirmwareNode* parentNode,ConstASCII8Str name,UInt32 cpuVersion,
	UInt32 clockFrequency,UInt32 busFrequency,UInt32 dCacheSize,UInt32 iCacheSize):
	OpenFirmwareNode(cpuNodeType,parentNode,name,"cpu")
{
	_myNode->cpuNode._cpuVersion = cpuVersion;
	_myNode->cpuNode._clockFrequency = clockFrequency;
	_myNode->cpuNode._busFrequency = busFrequency;
	_myNode->cpuNode._dCacheSize = dCacheSize;
	_myNode->cpuNode._iCacheSize = iCacheSize;
}

UInt32 OpenFirmwareCPUNode::cpuVersion()
{
	return _myNode->cpuNode._cpuVersion;
}

UInt32 OpenFirmwareCPUNode::clockFrequency()
{
	return _myNode->cpuNode._clockFrequency;
}

UInt32 OpenFirmwareCPUNode::busFrequency()
{
	return _myNode->cpuNode._busFrequency;
}

UInt32 OpenFirmwareCPUNode::dCacheSize()
{
	return _myNode->cpuNode._dCacheSize;
}

UInt32 OpenFirmwareCPUNode::iCacheSize()
{
	return _myNode->cpuNode._iCacheSize;
}

OpenFirmwareMemoryNode::OpenFirmwareMemoryNode(_OpenFirmwareNode* myNode,OpenFirmwareTree* myTree):
	OpenFirmwareNode(myNode,myTree)
{
}

OpenFirmwareMemoryNode::OpenFirmwareMemoryNode(OpenFirmwareNode* parentNode,ConstASCII8Str name,UInt32 baseAddr,UInt32 size):
	OpenFirmwareNode(memoryNodeType,parentNode,name,"memory")
{
	_myNode->memoryNode._baseAddr = baseAddr;
	_myNode->memoryNode._size = size;
}

UInt32 OpenFirmwareMemoryNode::baseAddr()
{
	return _myNode->memoryNode._baseAddr;
}

UInt32 OpenFirmwareMemoryNode::size()
{
	return _myNode->memoryNode._size;
}

OpenFirmwareCacheNode::OpenFirmwareCacheNode(_OpenFirmwareNode* myNode,OpenFirmwareTree* myTree):
	OpenFirmwareNode(myNode,myTree)
{
}

OpenFirmwareCacheNode::OpenFirmwareCacheNode(OpenFirmwareNode* parentNode,ConstASCII8Str name,UInt32 iCacheSize,
	UInt32 dCacheSize,UInt32 iCacheSets,UInt32 dCacheSets,Boolean unified):
	OpenFirmwareNode(cacheNodeType,parentNode,name,"cache")
{
	_myNode->cacheNode._iCacheSize = iCacheSize;
	_myNode->cacheNode._dCacheSize = dCacheSize;
	_myNode->cacheNode._iCacheSets = iCacheSets;
	_myNode->cacheNode._dCacheSets = dCacheSets;
	_myNode->cacheNode._unified = unified;
}

UInt32 OpenFirmwareCacheNode::iCacheSize()
{
	return _myNode->cacheNode._iCacheSize;
}

UInt32 OpenFirmwareCacheNode::dCacheSize()
{
	return _myNode->cacheNode._dCacheSize;
}

UInt32 OpenFirmwareCacheNode::iCacheSets()
{
	return _myNode->cacheNode._iCacheSets;
}

UInt32 OpenFirmwareCacheNode::dCacheSets()
{
	return _myNode->cacheNode._dCacheSets;
}

Boolean OpenFirmwareCacheNode::unified()
{
	return _myNode->cacheNode._unified;
}

OpenFirmwareDeviceNode::OpenFirmwareDeviceNode(_OpenFirmwareNode* myNode,OpenFirmwareTree* myTree):
	OpenFirmwareNode(myNode,myTree)
{
}

OpenFirmwareDeviceNode::OpenFirmwareDeviceNode(OpenFirmwareNode* parentNode,ConstASCII8Str name,ConstASCII8Str devType,UInt32 address0,UInt32 address1,
	UInt32 address2,UInt32 address3,UInt32 interrupt0,UInt32 interrupt1,UInt32 interrupt2,UInt32 interrupt3):
	OpenFirmwareNode(deviceNodeType,parentNode,name,devType)
{
	_myNode->deviceNode._address[0] = address0;
	_myNode->deviceNode._address[1] = address1;
	_myNode->deviceNode._address[2] = address2;
	_myNode->deviceNode._address[3] = address3;
	_myNode->deviceNode._interrupts[0] = interrupt0;
	_myNode->deviceNode._interrupts[1] = interrupt1;
	_myNode->deviceNode._interrupts[2] = interrupt2;
	_myNode->deviceNode._interrupts[3] = interrupt3;
}

UInt32 OpenFirmwareDeviceNode::address(UInt32 n)
{
	return _myNode->deviceNode._address[n];
}

UInt32 OpenFirmwareDeviceNode::interrupt(UInt32 n)
{
	return (_myNode->deviceNode._interrupts[n] == -1 ? 64 : _myNode->deviceNode._interrupts[n]);
}

OpenFirmwarePCINode::OpenFirmwarePCINode(_OpenFirmwareNode* myNode,OpenFirmwareTree* myTree):
	OpenFirmwareNode(myNode,myTree)
{
}

OpenFirmwarePCINode::OpenFirmwarePCINode(OpenFirmwareNode* parentNode,ConstASCII8Str name,UInt32 reg0,UInt32 reg1,UInt32 busRange0,UInt32 busRange1,UInt8* devFNList,UInt8* devIRQList,UInt8 numIRQs):
	OpenFirmwareNode(pciNodeType,parentNode,name,"pci")
{
	if(numIRQs > MAX_PCI_IRQ_COUNT)	// Silently fail
		numIRQs = MAX_PCI_IRQ_COUNT;
	
	_myNode->pciNode._reg[0] = reg0;
	_myNode->pciNode._reg[1] = reg1;
	_myNode->pciNode._busRange[0] = busRange0;
	_myNode->pciNode._busRange[1] = busRange1;
	_myNode->pciNode._numIRQs = numIRQs;
	for(UInt32 i=0;i<numIRQs;i++)
	{
		_myNode->pciNode._devFN[i] = devFNList[i];
		_myNode->pciNode._irq[i] = devIRQList[i];
	}
}

UInt32 OpenFirmwarePCINode::reg(UInt32 n)
{
	return _myNode->pciNode._reg[n];
}

UInt32 OpenFirmwarePCINode::busRange(UInt32 n)
{
	return _myNode->pciNode._busRange[n];
}

UInt32 OpenFirmwarePCINode::irq(UInt8 bus,UInt8 devFN)
{
	if(bus == _myNode->pciNode._busRange[0])
	{
		for(UInt32 i=0;i<_myNode->pciNode._numIRQs;i++)
		{
			if(_myNode->pciNode._devFN[i] == devFN)
				return _myNode->pciNode._irq[i];
		}
	}
	else if(bus > _myNode->pciNode._busRange[0] && bus <= _myNode->pciNode._busRange[1])
	{
		OpenFirmwareNode*	theNode = firstChild();
		while(theNode)
		{
			if(theNode->nodeType() == pciNodeType)
			{
				OpenFirmwarePCINode* pciNode = static_cast<OpenFirmwarePCINode*>(theNode);
				if(bus >= pciNode->busRange(0) && bus <= pciNode->busRange(1))
					return pciNode->irq(bus,devFN);
			}
			theNode = theNode->nextSibling();
		}
	}
	
	return 0xFFFFFFFF;
}

OpenFirmwareEmptyNode::OpenFirmwareEmptyNode(_OpenFirmwareNode* myNode,OpenFirmwareTree* myTree):
	OpenFirmwareNode(myNode,myTree)
{
}

OpenFirmwareEmptyNode::OpenFirmwareEmptyNode(OpenFirmwareNode* parentNode,ConstASCII8Str name,ConstASCII8Str devType):
	OpenFirmwareNode(emptyNodeType,parentNode,name,devType)
{
}
