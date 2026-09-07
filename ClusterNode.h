#ifndef MODELCLUSTERNODE_H
#define MODELCLUSTERNODE_H

#include <osg/Node>
#include <osg/Group>
#include <osg/Camera>
#include <osg/observer_ptr>
#include <osgEarth/MapNode>
#include <osgEarth/Horizon>
#include <vector>
#include "SceneCore/SceneCoreExport.h"

namespace CustomUtil
{
    /**
     * ModelClusterNode 基于屏幕空间的像素距离，对任意通用 3D 模型或节点进行聚类。
     * 聚类过程中，完全不修改子节点的任何属性（如 NodeMask）。
     * 它通过在 Cull 阶段有选择性地中止被聚合节点的遍历（不调用 accept），来实现视图安全的剔除优化。
     */
    class SCENECORE_API ModelClusterNode : public osg::Node
    {
    public:
        // 聚类簇结构
        struct Cluster
        {
            osg::ref_ptr< osg::Node > representativeNode; // 聚类簇中被选出的代表节点（将继续接受遍历渲染）
            //osg::NodeList hiddenNodes;                    // 簇中其他被折叠的节点（将中止遍历）
            int _neighborCount = 0;
        };

        typedef std::vector< Cluster > ClusterList;

        class CanClusterCallback : public osg::Referenced
        {
        public:
            virtual bool operator()(osg::Node* a, osg::Node* b) { return true; }
        };

        // 可以在此处让用户获取聚合结果，执行一些非修改节点本身（如生成 HUD 提示）的操作
        class OnClusterGeneratedCallback : public osg::Referenced
        {
        public:
            virtual void operator()(Cluster& cluster) {}
        };

    public:
        //bool showPlaceNodePoi是否显示聚合气泡
        ModelClusterNode(osgEarth::MapNode* mapNode, bool showPlaceNodePoi);

        void setMapNode(osgEarth::MapNode* mapNode);
        osgEarth::MapNode* getMapNode() const;

        void addNode(osg::Node* node);
        void removeNode(osg::Node* node);
        void clear();

        unsigned int getRadius() const;
        void setRadius(unsigned int radius);

        bool getEnabled() const;
        void setEnabled(bool enabled);

        void setCanClusterCallback(CanClusterCallback* callback);
        CanClusterCallback* getCanClusterCallback();

        void setOnClusterGeneratedCallback(OnClusterGeneratedCallback* callback);
        OnClusterGeneratedCallback* getOnClusterGeneratedCallback();

        // 核心：拦截遍历过程
        virtual void traverse(osg::NodeVisitor& nv);
        bool isShowPlaceNodePoi();
    protected:
        virtual ~ModelClusterNode() {}

        void getClusters(osgUtil::CullVisitor* cv, ClusterList& out);
        void buildIndex();

        osg::NodeList _nodes;
        osg::NodeList _clusterIndex;

        unsigned int _radius;
        bool _enabled;
        bool _dirtyIndex;
        bool _dirty;

        osg::observer_ptr< osgEarth::MapNode > _mapNode;
        osg::ref_ptr< CanClusterCallback > _canClusterCallback;
        osg::ref_ptr< OnClusterGeneratedCallback > _onClusterGeneratedCallback;
        osg::ref_ptr< osgEarth::Horizon > _horizon;

        osg::Matrixd _lastViewMatrix;
        ClusterList _clusters;
        int _lastClusterFrame = -100000;
        bool _showPlaceNodePoi = false;
    };
}

#endif // MODELCLUSTERNODE_H