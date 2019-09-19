/////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Tencent is pleased to support the open source community by making behaviac available.
//
// Copyright (C) 2015-2017 THL A29 Limited, a Tencent company. All rights reserved.
//
// Licensed under the BSD 3-Clause License (the "License"); you may not use this file except in compliance with
// the License. You may obtain a copy of the License at http://opensource.org/licenses/BSD-3-Clause
//
// Unless required by applicable law or agreed to in writing, software distributed under the License is
// distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and limitations under the License.
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#include "behaviac/agent/agent.h"

#include "behaviac/common/file/filesystem.h"

namespace behaviac {
	BehaviorTreeTask* ParseBTTask(IIONode* btNode);
	
    State_t::State_t() : m_bt(0) {
    }

    State_t::~State_t() {
        this->Clear();
    }

    State_t::State_t(const State_t& c) : m_bt(0) {
        *this = c;
    }

    State_t& State_t::operator=(const State_t& c) {
        c.m_vars.CopyTo(0, this->m_vars);

        if (c.m_bt) {
            const BehaviorNode* pNode = c.m_bt->GetNode();
            this->m_bt = (BehaviorTreeTask*)pNode->CreateAndInitTask();

            c.m_bt->CopyTo(this->m_bt);
        }

        return *this;
    }

    void State_t::Clear() {
        this->m_vars.Clear(true);

        BEHAVIAC_DELETE(this->m_bt);
        this->m_bt = 0;
    }

    XmlNodeReference State_t::SaveToXmlNode(Agent* pAgent) const {
        XmlNodeReference xmlInfo = CreateXmlNode("AgentState");

        CTextNode node(xmlInfo);

        CIOID  attrId("agentType");
        node.setAttr(attrId, this->m_agentType);

        if (pAgent) {
            CIOID  membersId("members");
            IIONode* membersNode = node.newNodeChild(membersId);
			CTagObject::Save(pAgent, membersNode, pAgent->GetObjectTypeName());
        }

        this->m_vars.Save(&node);

        if (this->m_bt) {
            this->m_bt->Save(&node);
        }

		CIOID  stacksId("stacks");
		IIONode* stacksNode = node.newNodeChild(stacksId);
    	
		for (vector<BehaviorTreeStackItem_t>::const_iterator it = this->m_btStack.begin(); it != this->m_btStack.end(); ++it) {
			BehaviorTreeStackItem_t item = *it;

			CIOID  itemId("item");
			IIONode* itemNode = stacksNode->newNodeChild(itemId);

			CIOID  eventId("event");
			itemNode->setAttr(eventId, item.triggerByEvent);

			item.bt->Save(itemNode);
		}
    	
        return xmlInfo;
    }

    void State_t::LoadFromXmlNode(CTextNode& node, Agent* pAgent) {
        if (pAgent) {
            CIOID  membersId("members");
            IIONode* membersNode = node.findNodeChild(membersId);
        	if (membersNode)
        	{
				CTagObject::Load(pAgent, membersNode, pAgent->GetObjectTypeName());
        	}
        }

        this->m_vars.Load(&node);

        CIOID  btNodeId("BehaviorTree");
        IIONode* btNode = node.findNodeChild(btNodeId);

    	if (btNode)
    	{
			BEHAVIAC_DELETE(this->m_bt);
			this->m_bt = ParseBTTask(btNode);
    	}

		CIOID  stacksNodeId("stacks");
		IIONode* stacksNode = node.findNodeChild(stacksNodeId);

		if (stacksNode)
		{
			this->m_btStack.clear();
			int32_t count = stacksNode->getChildCount();
			CIOID  eventId("event");

			for (int i = 0;i < count;i++)
			{
				behaviac::string attrStr;
				IIONode* item = stacksNode->getChild(i);
				item->getAttr(eventId, attrStr);
				IIONode* stackBtNode = item->findNodeChild(btNodeId);
				BehaviorTreeTask* stackBt = ParseBTTask(stackBtNode);
				bool triggerByEvent = false;
				StringUtils::ParseString(attrStr.c_str(), triggerByEvent);
				BehaviorTreeStackItem_t newItem(stackBt, triggerByEvent);
				this->m_btStack.push_back(newItem);
			}
		}
    }

    bool State_t::SaveToFile(const char* fileName, Agent* pAgent) const {
        XmlNodeReference xmlInfo = this->SaveToXmlNode(pAgent);
        CFileSystem::MakeSureDirectoryExist(fileName);
        return xmlInfo->saveToFile(fileName);
    }

    bool State_t::LoadFromFile(const char* fileName, Agent* pAgent) {
        XmlNodeReference xmlInfo = CreateXmlNode("AgentState");

        CTextNode node(xmlInfo);

        if (node.LoadFromFile(fileName)) {
            this->LoadFromXmlNode(node, pAgent);

            return true;
        }

        return false;
    }

    bool State_t::SaveToFile(IFile* file, Agent* pAgent) const {
		BEHAVIAC_UNUSED_VAR(file);
		BEHAVIAC_UNUSED_VAR(pAgent);

		//XmlNodeReference xmlInfo = this->SaveToXmlNode(pAgent);
        //return xmlInfo->saveToFile(file);
        return false;
    }

    bool State_t::LoadFromFile(IFile* file, Agent* pAgent) {
		BEHAVIAC_UNUSED_VAR(file);
		BEHAVIAC_UNUSED_VAR(pAgent);

        //XmlNodeReference xmlInfo = CreateXmlNode("AgentState");
        //CTextNode node(xmlInfo);

        //if (node.LoadFromFile(file))
        //{
        //    this->LoadFromXmlNode(node, pAgent);
        //    return true;
        //}

        return false;
    }

	BehaviorTreeTask* ParseBTTask(IIONode* btNode)
	{
		BehaviorTreeTask* bt = nullptr;
		CIOID  sourceId("source");
		behaviac::string btName;

		if (btNode->getAttr(sourceId, btName)) {
			bt = Workspace::GetInstance()->CreateBehaviorTreeTask(btName.c_str());
		}

		CIOID  nodeId("node");
		IIONode* n = btNode->findNodeChild(nodeId);
		BEHAVIAC_ASSERT(n);

		if (bt && n) {
			bt->Load(n);
		}
		return bt;
	}
}
