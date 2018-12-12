import os.path
import sys
import fbx

class JointData:
	def __init__(self):
		self.transform = fbx.FbxAMatrix();
		self.name = "null";
		self.parent = 0;

def JointsDFS(node, names, parents, parentName):
	for i in range(0, node.GetChildCount()):
		names.append(node.GetChild(i).GetName())
		parents.append(parentName)
		JointsDFS(node.GetChild(i), names, parents, node.GetChild(i).GetName())

def StoreSkeletonToFile(scene, meshName, joints):
	assetsOut = os.path.join(sys.path[0], "Export", meshName)
	names = []
	parents = []
	selfIndex = []
	
	childNum = scene.GetRootNode().GetChildCount()
	print("Total child num: %i"%childNum)
	
	for n in range(0, childNum):
		node = scene.GetRootNode().GetChild(n)
		mesh = node.GetMesh()
		if (mesh is None):
			continue
		skinDeformer = mesh.GetDeformer(0)
		print(node.GetName())
	
	for n in range(0, childNum):
		node = scene.GetRootNode().GetChild(n)
		print("Child name: %s"%node.GetName())
		skeleton = node.GetSkeleton()
		if (skeleton is None):
			continue
		
		clusterNum = skinDeformer.GetClusterCount();
		scaleFactor = 1.0 / skinDeformer.GetCluster(0).GetLink().EvaluateLocalTransform().GetS()[0]
		print(scaleFactor)
		
		# Cluster refers to joint in this context
		for i in range(0, clusterNum):
			currCluster = skinDeformer.GetCluster(i)
			# Get skeleton
			# Assumed global transform is freezed, i.e. identity
			transform = fbx.FbxAMatrix()
			linkTransform = fbx.FbxAMatrix()
			currCluster.GetTransformMatrix(transform)
			currCluster.GetTransformLinkMatrix(linkTransform)
			joints.append(JointData())
			joints[i].transform = transform * linkTransform.Inverse()
			#scale = joints[i].transform.GetS()
			scale = joints[i].transform.GetS()
			scale.Set(scaleFactor, scaleFactor, scaleFactor)
			joints[i].transform.SetS(scale)
			rotate = joints[i].transform.GetR()
			rotate.Set(rotate[0], -rotate[1], -rotate[2])
			joints[i].transform.SetR(rotate)
			translate = joints[i].transform.GetT()
			translate.Set(-translate[0] * scaleFactor, translate[1]  * scaleFactor, translate[2]  * scaleFactor)
			joints[i].transform.SetT(translate)
		
		
		selfIndex = range(0, len(joints))
		names.append(node.GetName())
		parents.append("null")
		JointsDFS(node, names, parents, node.GetName())
		joints[0].name = names[0]
		joints[0].parent = -1
		for j in range(1, len(joints)):
			joints[j].name = names[j]
			joints[j].parent = names.index(parents[j])
		
		#write skeleton
		skeletonFileNames = '%s_skeleton.bufa'%(meshName)
		
		dirPath = os.path.join(assetsOut, "Skeleton")
		if not os.path.exists(dirPath):
			print("Creating %s folder" % dirPath)
			os.mkdir(dirPath)
		
		sbf = open(os.path.join(dirPath, skeletonFileNames), 'w')
		sbf.write("SKELETON\n%d\n"%(len(joints)))
		for j in joints:
			sbf.write("%s\n"%j.name)
			sbf.write("%f %f %f %f %f %f %f %f %f %f %f %f %f %f %f %f\n"%(j.transform.Get(0, 0), j.transform.Get(0, 1) , j.transform.Get(0, 2) , j.transform.Get(0, 3),j.transform.Get(1, 0) , j.transform.Get(1, 1) , j.transform.Get(1, 2) , j.transform.Get(1, 3) , j.transform.Get(2, 0) , j.transform.Get(2, 1) , j.transform.Get(2, 2) , j.transform.Get(2, 3) , j.transform.Get(3, 0) , j.transform.Get(3, 1) , j.transform.Get(3, 2) , j.transform.Get(3, 3)))
			sbf.write("%i\n"%j.parent)
		sbf.close()	