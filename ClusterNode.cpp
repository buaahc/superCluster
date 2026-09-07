



#include <osgUtil/CullVisitor>
#include <osgEarth/CullingUtils>
#include <osgEarthUtil/kdbush.hpp>
#include "ClusterNode.h"
#include "sceneNodeVisitor.h"
#include "sceneConfig.h"
#include "corePipeline.h"
#include "SceneCore/CommonData/BaseData.h"
#include "SceneCore/mutilPointModel/baseModel.h"


typedef std::pair<int, int> TPoint;
typedef std::vector< std::size_t > TIds;

namespace CustomUtil
{

    ModelClusterNode::ModelClusterNode(osgEarth::MapNode* mapNode, bool showPlaceNodePoi) :
        _radius(50),
        _mapNode(mapNode),
        _enabled(true),
        _dirty(true),
        _dirtyIndex(true),
        _showPlaceNodePoi(showPlaceNodePoi)
    {
        setCullingActive(false);
        _horizon = new osgEarth::Horizon();
    }

    void ModelClusterNode::addNode(osg::Node* node)
    {
        _nodes.push_back(node);
        _dirty = true;
        _dirtyIndex = true;
    }

    void ModelClusterNode::removeNode(osg::Node* node)
    {
        osg::NodeList::iterator itr = std::find(_nodes.begin(), _nodes.end(), node);
        if (itr != _nodes.end())
        {
            _nodes.erase(itr);
        }
        _dirty = true;
        _dirtyIndex = true;
    }

    void ModelClusterNode::clear()
    {
        _nodes.clear();
        _dirty = true;
        _dirtyIndex = true;
    }

    unsigned int ModelClusterNode::getRadius() const { return _radius; }
    void ModelClusterNode::setRadius(unsigned int radius) { _radius = radius; _dirty = true; }

    bool ModelClusterNode::getEnabled() const { return _enabled; }
    void ModelClusterNode::setEnabled(bool enabled) { _enabled = enabled; _dirty = true; }

    osgEarth::MapNode* ModelClusterNode::getMapNode() const { return _mapNode.get(); }
    void ModelClusterNode::setMapNode(osgEarth::MapNode* mapNode)
    {
        if (_mapNode != mapNode)
        {
            _mapNode = mapNode;
            _dirty = true;
            _dirtyIndex = true;
        }
    }

    void ModelClusterNode::setCanClusterCallback(CanClusterCallback* callback) { _canClusterCallback = callback; _dirty = true; }
    ModelClusterNode::CanClusterCallback* ModelClusterNode::getCanClusterCallback() { return _canClusterCallback.get(); }

    void ModelClusterNode::setOnClusterGeneratedCallback(OnClusterGeneratedCallback* callback) { _onClusterGeneratedCallback = callback; _dirty = true; }
    ModelClusterNode::OnClusterGeneratedCallback* ModelClusterNode::getOnClusterGeneratedCallback() { return _onClusterGeneratedCallback.get(); }

    bool boundSort(const osg::ref_ptr< osg::Node>& i, const osg::ref_ptr< osg::Node>& j)
    {
        return i->getBound().center().x() < j->getBound().center().x();
    }

    void ModelClusterNode::buildIndex()
    {
        if (_dirtyIndex)
        {
            _clusterIndex.clear();
            std::sort(_nodes.begin(), _nodes.end(), boundSort);

            unsigned int maxNodes = 10000;
            osg::Group* currentGroup = 0;

            for (unsigned int i = 0; i < _nodes.size(); i++)
            {
                if (!currentGroup || currentGroup->getNumChildren() >= maxNodes)
                {
                    currentGroup = new osg::Group;
                    _clusterIndex.push_back(currentGroup);
                }
                currentGroup->addChild(_nodes[i]);
            }
        }
        _dirtyIndex = false;
    }


    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

    // 辅助结构体，用于在排序和聚合时绑定节点状态
    struct ClusterCandidate {
        osg::Node* node;
        CPickMatrixTransform* worldMT;
        int priority;
        TPoint screenPos;
        bool isClustered;

        ClusterCandidate(osg::Node* n, CPickMatrixTransform* mt, int p, const TPoint& sp)
            : node(n), worldMT(mt), priority(p), screenPos(sp), isClustered(false) {
        }
    };

    void ModelClusterNode::getClusters(osgUtil::CullVisitor* cv, ClusterList& out)
    {
        osg::Camera* camera = cv->getCurrentCamera();
        osg::Viewport* viewport = camera->getViewport();
        if (!viewport) return;

        osg::Matrixd projMatrix = camera->getProjectionMatrix();
        if (CConfig::Get()->versePipeline())
            projMatrix = syzCorePipeLine::Get()->_doubleViewData.projectionMatrix;

        osg::Matrixd viewMatrix = camera->getViewMatrix();
        osg::Matrixd pwMatrix= projMatrix * viewport->computeWindowMatrix();

        std::vector<ClusterCandidate> candidates;
        candidates.reserve(_nodes.size());

        buildIndex();

        for (osg::NodeList::iterator itr = _clusterIndex.begin(); itr != _clusterIndex.end(); ++itr)
        {
            osg::Group* index = static_cast<osg::Group*>(itr->get());

            if (cv->isCulled(index->getBound())) continue;
            const unsigned int numChildren = index->getNumChildren();
            for (unsigned int i = 0; i < numChildren; i++)
            {
                osg::Node* node = index->getChild(i);

                if (!node || (node->getNodeMask() & cv->getTraversalMask()) == 0 || cv->isCulled(*node)) continue;

                CPickMatrixTransform* worldMT = static_cast<CPickMatrixTransform*>(node);
                
                // 默认隐藏，后面聚合时再设为 true
                worldMT->_showClusterPoi = false;

                osg::Vec3d world = worldMT->getMatrix().getTrans();
                if (_horizon.valid() && !_horizon->isVisible(world)) continue;

                osg::Vec3d viewPos = world * viewMatrix;
                bool zValid = viewPos.z() < 0;
                if (!zValid)
                    continue;
                osg::Vec3d screen = viewPos * pwMatrix;

                // 视口内剔除
                if (screen.x() >= 0 && screen.x() <= viewport->width() &&
                    screen.y() >= 0 && screen.y() <= viewport->height())
                {
                    // 获取优先级
                    int priority = worldMT->_clusterPriority;
                    candidates.emplace_back(node, worldMT, priority, TPoint(static_cast<int>(screen.x()), static_cast<int>(screen.y())));
                }
            }
        }

        if (candidates.empty()) return;

        // 按优先级绝对降序排序
        std::sort(candidates.begin(), candidates.end(), [](const ClusterCandidate& a, const ClusterCandidate& b) {
            return a.priority > b.priority;
            });

        std::vector<TPoint> pointsForKD;
        pointsForKD.reserve(candidates.size());
        for (const auto& c : candidates) {
            pointsForKD.push_back(c.screenPos);
        }

        kdbush::KDBush<TPoint> kdIndex(pointsForKD);

        TIds neighborIndices;
        neighborIndices.reserve(128);


        //按照优先级从高到低排序好的nodes
        for (size_t i = 0; i < candidates.size(); ++i)
        {
            // 如果当前节点已经被聚合，直接跳过
            if (candidates[i].isClustered) continue;

            candidates[i].worldMT->_showClusterPoi = true;
            candidates[i].isClustered = true;

            // 高效复用 vector，只清空 size，保留 capacity，0 开销
            neighborIndices.clear();

            kdIndex.range(candidates[i].screenPos.first - _radius, candidates[i].screenPos.second - _radius,
                candidates[i].screenPos.first + _radius, candidates[i].screenPos.second + _radius,
                neighborIndices);

            const size_t neighborCount = neighborIndices.size();
            int actualClusteredCount = 0;//actualClusteredCount去重，包括去掉自己的真实周边聚合数量
            for (size_t j = 0; j < neighborCount; ++j)
            {
                size_t neighborIdx = neighborIndices[j];
                if (!candidates[neighborIdx].isClustered)
                {
                    if (_canClusterCallback.valid() &&
                        !(*_canClusterCallback)(candidates[i].node, candidates[neighborIdx].node))
                    {
                        continue;
                    }
                    candidates[neighborIdx].isClustered = true;
                    actualClusteredCount++; // 真正聚合成功，数量+1
                }
            }

            Cluster cluster;
            cluster._neighborCount = actualClusteredCount;
            cluster.representativeNode = candidates[i].node;
            out.push_back(cluster);

            //显示气泡poi
            if(this->_showPlaceNodePoi)
            {
                CPickMatrixTransform* worldMT = static_cast<CPickMatrixTransform*>(cluster.representativeNode.get());
                if (cluster._neighborCount > 0)
                {
                    if (worldMT->_clusterShowText.valid())
                    {
                        worldMT->_clusterShowText->setText(std::to_string(cluster._neighborCount + 1), osgText::String::ENCODING_UTF8);
                    }
                    worldMT->_traverseIveMT = false;
                }
                else
                {
                    worldMT->_traverseIveMT = true;
                }
            }
        }
    }
    //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

    void ModelClusterNode::traverse(osg::NodeVisitor& nv)
    {
        // 只有在 Cull 阶段才执行聚类计算和拦截
        if (nv.getVisitorType() == osg::NodeVisitor::CULL_VISITOR)
        {
            osgUtil::CullVisitor* cv = osgEarth::Culling::asCullVisitor(nv);

            if (!_enabled)
            {
                // 如果功能关闭，回退到普通 Group 的行为，全部接受遍历
                for (osg::NodeList::iterator itr = _nodes.begin(); itr != _nodes.end(); ++itr)
                {
                    itr->get()->accept(nv);
                }
                return;
            }

            if (_mapNode.valid())
            {
                const osg::Matrixd& currentViewMatrix = cv->getCurrentCamera()->getViewMatrix();

                osg::CullingSet::Mask cullingMask = cv->getCurrentCullingSet().getCullingMask();
                //显示气泡poi,去掉细节裁剪
                if (this->_showPlaceNodePoi)
                {
                    osg::CullingSet::Mask noSmallPixelCullingMask = cullingMask & (~osg::CullingSet::MaskValues::SMALL_FEATURE_CULLING);
                    cv->getCurrentCullingSet().setCullingMask(noSmallPixelCullingMask);
                }

                // 缓存聚类结果，在相机移动或者_dirty = true时重算
                if (
                    ((this->_lastViewMatrix != currentViewMatrix) && (cv->getFrameStamp()->getFrameNumber() - this->_lastClusterFrame > CConfig::_deltaClusterFrame)) ||
                    this->_dirty
                    )
                {
                    this->_lastClusterFrame = cv->getFrameStamp()->getFrameNumber();
                    osg::Vec3d eye, center, up;
                    cv->getCurrentCamera()->getViewMatrixAsLookAt(eye, center, up);
                    _horizon->setEye(eye);

                    _clusters.clear();
                    getClusters(cv, _clusters);
                    // 触发用户回调
                    if (_onClusterGeneratedCallback)
                    {
                        for (ClusterList::iterator itr = _clusters.begin(); itr != _clusters.end(); ++itr)
                        {
                            (*_onClusterGeneratedCallback)(*itr);
                        }
                    }

                    this->_dirty = false;
                    this->_lastViewMatrix = currentViewMatrix;
                }

                for (ClusterList::iterator itr = _clusters.begin(); itr != _clusters.end(); ++itr)
                {
                    if (itr->representativeNode.valid())
                    {
                        itr->representativeNode->accept(nv);
                    }
                }

                if (this->_showPlaceNodePoi)
                {
                    //去掉细节裁切
                    cv->getCurrentCullingSet().setCullingMask(cullingMask);
                }

            }
        }
        else
        {
            // 对于 UpdateVisitor、EventVisitor 等非裁剪遍历器
            // 我们必须将遍历传递给所有的节点，以保证动画、事件处理等底层机制正常运作
            for (osg::NodeList::iterator itr = _nodes.begin(); itr != _nodes.end(); ++itr)
            {
                itr->get()->accept(nv);
            }
        }
    }

    bool ModelClusterNode::isShowPlaceNodePoi()
    {
        return this->_showPlaceNodePoi;
    }

} // namespace CustomUtil