
import sys
import fbx
import os

from StoreMeshToFile import StoreMeshToFile
from StoreSkeletonToFile import StoreSkeletonToFile
from StoreAnimationToFile import StoreAnimationToFile

def l(*vartuple):
    filename = "church"
    filepath = os.path.join(sys.path[0], filename + ".fbx")
    print(filepath)
    manager = fbx.FbxManager.Create()
    importer = fbx.FbxImporter.Create(manager, 'myImporter')
    importer.Initialize(filepath)
    scene = fbx.FbxScene.Create(manager, 'myScene')
    importer.Import(scene)
    importer.Destroy()
    	
    converter = fbx.FbxGeometryConverter(manager)
    res = converter.Triangulate(scene, True)
    	
    dirPath = os.path.join(sys.path[0], "Export", filename)
    if not os.path.exists(dirPath):
        os.mkdir(dirPath)

    joints = []
    
    StoreMeshToFile(scene, filename)
   # StoreSkeletonToFile(scene, filename, joints)
   # StoreAnimationToFile(scene, filename, joints)

    print("Finished!")
    manager.Destroy()
    sys.exit(0)
