package com.vperflab.tsserver

import scala.collection.immutable._
import scala.collection.mutable.{Map => MutableMap}

/*
 * TSObject definitions and serialization/deserialization
 * 
 * Contains classes that is used to exchange information between TSServer and tsload daemon
 * 
 * Each class is a sublass of TSObject which may have these types of fields:
 * - String
 * - Long or Double
 * - another TSObject
 * - Map[String, TSObject] or List[TSObject]
 * 
 * Each Map or List should annotated with @TSObjContainer(elementClass = classOf[TSObjectElementClass])
 * where TSObjectElementClass - class of elements which could be stored in Map or List
 * 
 * TSObject may be abstract, if so it should be annotated with @TSObjAbstract(fieldName = "type",
    classMap = Array("integer", "Integer", 
     			    "double", "Double")
    
 *	where fieldName is a key, which corresponding value should be used for class construction.
 * classMap is an array of strings, which represent possible values and corresponding classNames
 * 
 * In this example: for {"type": "integer", "value": 10}, new Integer(10) will be constructed
 */

abstract class TSObject