package com.vperflab.tsserver;

import com.codahale.jerkson.Json._

import org.junit._
import Assert._

import scala.collection.immutable._

class TSObjectDeserializerTest extends Assert {
  val testJsonStr : String = """{"modules":{"dummy":{"path":"","params":{"test":{"type":"strset","strset":["read","write"]},"filesize":{"type":"size","max":1099511627776,"min":1},"sparse":{"type":"bool"},"path":{"type":"string","len":512},"blocksize":{"type":"size","max":16777216,"min":1}}}}}"""

  @Test
  def testSerdes() {
    val jsonMap = parse[Map[String, Any]](testJsonStr) 
    
	var obj = TSObjectDeserializer.doDeserialize(jsonMap, classOf[TSModulesInfo])
	var modulesInfo = obj.asInstanceOf[TSModulesInfo]  
	val newJsonMap = TSObjectSerializer.doSerialize(modulesInfo)
	val newJsonStr = generate(newJsonMap)
	
	assertEquals(testJsonStr, newJsonStr)
  }
  
  @Test
  def testSerArrayAny() {
    var workload = new TSWorkload()
    
    workload.module = "dummy"
    workload.params = Map("filesize" -> 16777216,
        "blocksize" -> 4096,
        "path" -> "/tmp/testfile",
        "test" -> "read",
        "sparse" -> false)
    
    try {
      val wlJsonMap = TSObjectSerializer.doSerialize(workload)
		val wlJsonStr = generate(wlJsonMap)
		
		System.out.println(wlJsonStr)
    }
    catch {
    case e: Exception => { 
    	  e.printStackTrace()
    	  throw e 
      }
    }
    
  }
}