import {describe,expect,it} from "vitest";
import {NEUTRAL_POSE,WORKER_PROTOCOL_VERSION,isMandibularPose,isPhysicsWorkerRequest,isPhysicsWorkerResponse,metersToMillimeters} from "./worker-contract";
const transform={translationMeters:{x:0,y:-.09,z:0},rotationQuaternion:{x:0,y:0,z:0,w:1}};
const separated={id:"p",type:"mandibular-pose-result",ok:true,fixtureId:"f",sequence:1,requestedPose:NEUTRAL_POSE,appliedTransform:transform,classification:"separated",measurementStatus:"unavailable-separated",clearanceMeters:null,penetrationDepthMeters:0,contactCount:0,contactSamples:[]};
describe("physics worker protocol v3",()=>{
 it("validates every response variant",()=>{expect(isPhysicsWorkerResponse({id:"h",type:"health",ok:true,protocolVersion:WORKER_PROTOCOL_VERSION,rapierVersion:"r",fixtureName:"f"})).toBe(true);expect(isPhysicsWorkerResponse({id:"r",type:"fixture-ready",ok:true,fixtureId:"f",protocolVersion:WORKER_PROTOCOL_VERSION})).toBe(true);expect(isPhysicsWorkerResponse(separated)).toBe(true);expect(isPhysicsWorkerResponse({id:"e",type:"error",ok:false,code:"not-initialized",message:"initialize"})).toBe(true);});
 it.each([NaN,Infinity,-Infinity,-.01,.251])("rejects invalid opening %s",openingMeters=>expect(isMandibularPose({...NEUTRAL_POSE,openingMeters})).toBe(false));
 it("rejects unknown requests",()=>expect(isPhysicsWorkerRequest({id:"x",type:"invent-result"})).toBe(false));
 it("rejects malformed samples and quaternions",()=>{expect(isPhysicsWorkerResponse({...separated,appliedTransform:{...transform,rotationQuaternion:{x:0,y:0,z:0,w:2}}})).toBe(false);expect(isPhysicsWorkerResponse({...separated,contactSamples:[{pointWorldMeters:{x:NaN,y:0,z:0}}]})).toBe(false);});
 it("rejects touching results whose samples imply penetration",()=>{const sample={id:"c",pointWorldMeters:{x:0,y:0,z:0},normalWorld:{x:0,y:1,z:0},signedDistanceMeters:-.000001,penetrationDepthMeters:.000001,surfaces:["maxilla","mandible"],units:"meters"};expect(isPhysicsWorkerResponse({...separated,classification:"touching",measurementStatus:"rapier-contact",clearanceMeters:0,contactCount:1,contactSamples:[sample]})).toBe(false);});
 it("converts internal meters to displayed millimeters",()=>expect(metersToMillimeters(.005)).toBe(5));
});
