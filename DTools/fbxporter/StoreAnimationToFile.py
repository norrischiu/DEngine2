import os.path
import sys
import fbx

class AnimationData:
	def __init__(self):
		self.pose = [];
		self.quat = [];
		self.name = "null";
		
def StoreAnimationToFile(scene, meshName, joints):
	assetsOut = os.path.join(sys.path[0], "Export", meshName)
	animations = []
	
	childNum = scene.GetRootNode().GetChildCount()
	for n in range(0, childNum):
		node = scene.GetRootNode().GetChild(n)
		mesh = node.GetMesh()
		if (mesh is None):
			continue
		skinDeformer = mesh.GetDeformer(0)
	
	animStack = scene.GetSrcObject(fbx.FbxAnimStack.ClassId)
	timespan = animStack.GetLocalTimeSpan()
	startFrame = timespan.GetStart().GetFrameCount(fbx.FbxTime.eFrames30)
	endFrame = 17#timespan.GetStop().GetFrameCount(fbx.FbxTime.eFrames30)
	clusterNum = skinDeformer.GetClusterCount()
	print("Start frame: %i"%startFrame)
	print("End frame: %i"%endFrame)

	animationFileNames = '%s_animation.bufa'%(meshName)
	
	dirPath = os.path.join(assetsOut, "Animations")
	if not os.path.exists(dirPath):
		print("Creating %s folder" % dirPath)
		os.mkdir(dirPath)
	
	af = open(os.path.join(dirPath, animationFileNames), 'w')
	af.write("ANIMATION_CLIP\njump\n%d\n"%(len(joints)))
	af.write("%i\n"%(endFrame - startFrame + 1))
	
	scaleFactor = skinDeformer.GetCluster(0).GetLink().EvaluateLocalTransform().GetS()[0]
	for i in range(0, clusterNum):
		animations.append(AnimationData())
		animations[i].name = joints[i].name
		af.write("%s\n"%joints[i].name)
		currLink = skinDeformer.GetCluster(i).GetLink()
		rotationOrder = currLink.RotationOrder.Get()
		translateMat = fbx.FbxAMatrix()
		rotationMat = fbx.FbxAMatrix()
		rotationMatX = fbx.FbxAMatrix()
		rotationMatY = fbx.FbxAMatrix()
		rotationMatZ = fbx.FbxAMatrix()
		for t in range(startFrame, endFrame + 1):
			localPose = fbx.FbxAMatrix()
			currTime = fbx.FbxTime()
			currTime.SetFrame(t, fbx.FbxTime.eFrames30)
			localPose = currLink.EvaluateLocalTransform(currTime)
			rotate = localPose.GetR()
			rotationMatX.SetR(fbx.FbxVector4(rotate[0], 0, 0))
			rotationMatY.SetR(fbx.FbxVector4(0, -rotate[1], 0))
			rotationMatZ.SetR(fbx.FbxVector4(0, 0, -rotate[2]))
			if rotationOrder == 0: #XYZ
				rotationMat = rotationMatZ * rotationMatY * rotationMatX
			elif rotationOrder == 1: #XZY
				rotationMat = rotationMatY * rotationMatZ * rotationMatX
			elif rotationOrder == 2: #YZX
				rotationMat = rotationMatX * rotationMatZ * rotationMatY
			elif rotationOrder == 3: #YXZ
				rotationMat = rotationMatZ * rotationMatX * rotationMatY
			elif rotationOrder == 4: #ZXY
				rotationMat = rotationMatY * rotationMatX * rotationMatZ
			elif rotationOrder == 5: #ZYX
				rotationMat = rotationMatX * rotationMatY * rotationMatZ
			localPose.SetR(rotationMat.GetR())
			if (i != 0):
				scale = localPose.GetS()
				scale.Set(1.0, 1.0, 1.0)
				localPose.SetS(scale)
			translate = localPose.GetT()
			translate.Set(-translate[0], translate[1] , translate[2] )
			localPose.SetT(translate)
			af.write("%f %f %f %f \n"%(localPose.GetQ()[0], localPose.GetQ()[1], localPose.GetQ()[2], localPose.GetQ()[3]))
			af.write("%f %f %f \n"%(localPose.GetT()[0], localPose.GetT()[1], localPose.GetT()[2]))
			af.write("%f\n"%(localPose.GetS()[0]))
			#af.write("%f %f %f %f %f %f %f %f %f %f %f %f %f %f %f %f\n"%(localPose.Get(0, 0), localPose.Get(0, 1) , localPose.Get(0, 2) , localPose.Get(0, 3),localPose.Get(1, 0) , localPose.Get(1, 1) , localPose.Get(1, 2) , localPose.Get(1, 3) , localPose.Get(2, 0) , localPose.Get(2, 1) , localPose.Get(2, 2) , localPose.Get(2, 3) , localPose.Get(3, 0) , localPose.Get(3, 1) , localPose.Get(3, 2) , localPose.Get(3, 3)))
			animations[i].pose.append(localPose)
			animations[i].quat.append(localPose.GetQ())
	
	'''for anim in animations:
		af.write("%s\n"%anim.name)
		for i in anim.pose:
			#af.write("%f %f %f %f \n"%(anim.quat[i][0], anim.quat[i][1], anim.quat[i][2], anim.quat[i][3]))
			#af.write("%f %f %f \n"%(i.GetT()[0], i.GetT()[1], i.GetT()[2]))
			#af.write("%f\n"%(i.GetS()[0]))
			af.write("%f %f %f %f %f %f %f %f %f %f %f %f %f %f %f %f\n"%(i.Get(0, 0), i.Get(0, 1) , i.Get(0, 2) , i.Get(0, 3),i.Get(1, 0) , i.Get(1, 1) , i.Get(1, 2) , i.Get(1, 3) , i.Get(2, 0) , i.Get(2, 1) , i.Get(2, 2) , i.Get(2, 3) , i.Get(3, 0) , i.Get(3, 1) , i.Get(3, 2) , i.Get(3, 3)))'''
	af.close()	