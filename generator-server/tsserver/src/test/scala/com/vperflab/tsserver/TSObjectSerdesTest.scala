package com.vperflab.tsserver;

import com.codahale.jerkson.Json._

import org.junit._
import Assert._

import scala.collection.immutable._

import java.lang.{Boolean => JBoolean, Integer => JInteger, Double => JDouble}

class TSObjectDeserializerTest extends Assert {
  val testJsonStr : String = """{"modules":{"dummy":{"path":"","params":{"test":{"type":"strset","strset":["read","write"]},"filesize":{"type":"size","max":1099511627776,"min":1},"sparse":{"type":"bool"},"path":{"type":"string","len":512},"blocksize":{"type":"size","max":16777216,"min":1}}}}}"""

  @Test
  def testSerdes() {
    val jsonMap = parse[Map[String, Any]](testJsonStr) 
    
	var obj = TSObjectDeserializer.doDeserialize(jsonMap, classOf[TSModulesInfo])
	var modulesInfo = obj.asInstanceOf[TSModulesInfo]  
	val newJsonMap = TSObjectSerializer.doSerializeObject(modulesInfo)
	val newJsonStr = generate(newJsonMap)
	
	assertEquals(testJsonStr, newJsonStr)
  }
  
  @Test
  def testSerArrayAny() {
    var workload = new TSWorkload()
    
    workload.module = "dummy"
    workload.params = Map("filesize" -> (16777216 : JInteger),
        "blocksize" -> (4096 : JInteger),
        "path" -> "/tmp/testfile",
        "test" -> "read",
        "sparse" -> (false : JBoolean))
    
   
   val wlJsonMap = TSObjectSerializer.doSerializeObject(workload)
   val wlJsonStr = generate(wlJsonMap)
		
   assertEquals(wlJsonStr, """{"module":"dummy","params":{"test":"read","filesize":16777216,"sparse":false,"path":"/tmp/testfile","blocksize":4096}}""")
  }
}