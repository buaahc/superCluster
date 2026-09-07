#include <osgViewer/Viewer>
#include <osgViewer/ViewerEventHandlers>
#include "ClusterNode.h"

void main()
{
	osg::ref_ptr<osgViewer::Viewer>pViewer = new osgViewer::Viewer;
	osg::ref_ptr<CustomUtil::ModelClusterNode>modelClusterNode = new CustomUtil::ModelClusterNode;
	//modelClusterNode->setEnabled(false);
	osg::ref_ptr<osg::Node>cessnaNode = osgDB::readNodeFile("cessna.ive");
	if (!cessnaNode)
		return;
	double radius = cessnaNode->getBound().radius();
	for (size_t i = 0; i < 300; i++)
	{
		for (size_t j = 0; j < 300; j++)
		{
			osg::ref_ptr<osg::MatrixTransform>mt = new CustomUtil::CPickMatrixTransform;
			mt->setMatrix(osg::Matrix::translate(osg::Vec3(i * radius * 2., j * radius * 1.5, 0.0)));
			mt->addChild(cessnaNode);
			modelClusterNode->addNode(mt);
		}
	}
	pViewer->setSceneData(modelClusterNode);
	pViewer->addEventHandler(new osgViewer::WindowSizeHandler);
	pViewer->addEventHandler(new osgViewer::StatsHandler);
	pViewer->run();
}