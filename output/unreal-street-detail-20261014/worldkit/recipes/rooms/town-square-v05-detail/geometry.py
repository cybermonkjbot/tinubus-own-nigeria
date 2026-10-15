"""Small editable construction helpers, with metric UVs."""
import bpy, math
import numpy as np
from mathutils import Vector

def material(name,color,rough=.6,metal=0,transmission=0):
    m=bpy.data.materials.new(name);m.use_nodes=True
    p=m.node_tree.nodes.get('Principled BSDF');p.inputs['Base Color'].default_value=(*color,1)
    p.inputs['Roughness'].default_value=rough;p.inputs['Metallic'].default_value=metal
    p.inputs['Transmission Weight'].default_value=transmission
    return m

class Builder:
    def __init__(self,collection):self.collection=collection
    def mesh(self,name,vertices,faces,mat,smooth=False,uvs=None):
        data=bpy.data.meshes.new(name);data.from_pydata(vertices,[],faces);data.update()
        data.materials.append(mat);ob=bpy.data.objects.new(name,data);self.collection.objects.link(ob)
        uv=data.uv_layers.new(name='SurfaceUV')
        for p in data.polygons:
            p.use_smooth=smooth
            axis=max(range(3),key=lambda i:abs(p.normal[i]));axes=(1,2) if axis==0 else (0,2) if axis==1 else (0,1)
            for li in p.loop_indices:
                index=data.loops[li].vertex_index;v=data.vertices[index].co
                uv.data[li].uv=uvs[index] if uvs is not None else (v[axes[0]]/2,v[axes[1]]/2)
        return ob
    def box(self,name,loc,size,mat,bevel=.008):
        c=Vector(loc);x,y,z=[v/2 for v in size]
        vertices=[tuple(c+Vector(p)) for p in [(-x,-y,-z),(-x,-y,z),(-x,y,-z),(-x,y,z),(x,-y,-z),(x,-y,z),(x,y,-z),(x,y,z)]]
        ob=self.mesh(name,vertices,[(2,6,4,0),(5,7,3,1),(4,5,1,0),(3,7,6,2),(1,3,2,0),(6,7,5,4)],mat)
        if bevel:
            mod=ob.modifiers.new('Manufactured edge radius','BEVEL');mod.width=bevel;mod.segments=3
            ob.modifiers.new('Plane normals','WEIGHTED_NORMAL')
        return ob
    def tube(self,name,points,radii,mat,sides=12):
        points=[Vector(p) for p in points];vertices=[];uvs=[];length=0
        previous=None
        for i,p in enumerate(points):
            tangent=(points[min(i+1,len(points)-1)]-points[max(0,i-1)]).normalized()
            u=tangent.cross(Vector((0,0,1)))
            if u.length<.01:u=tangent.cross(Vector((0,1,0)))
            u.normalize();v=tangent.cross(u).normalized()
            if previous is not None and u.dot(previous)<0:u=-u;v=-v
            previous=u
            if i:length+=(p-points[i-1]).length
            radius=radii[i] if hasattr(radii,'__len__') else radii
            for j in range(sides):
                angle=j*math.tau/sides;vertices.append(tuple(p+radius*(u*math.cos(angle)+v*math.sin(angle))))
                uvs.append((j/sides,length))
        faces=[tuple(reversed(range(sides))),tuple(range((len(points)-1)*sides,len(points)*sides))]
        for i in range(len(points)-1):
            for j in range(sides):faces.append((i*sides+j,i*sides+(j+1)%sides,(i+1)*sides+(j+1)%sides,(i+1)*sides+j))
        return self.mesh(name,vertices,faces,mat,True,uvs)
    def rod(self,name,a,b,r,mat,sides=16):return self.tube(name,[a,b],r,mat,sides)

def archive_collection(root,collection,label):
    archive=bpy.data.collections.new(label);archive.use_fake_user=True
    root.children.unlink(collection);archive.children.link(collection)
    return archive
