package com.vperflab.tsserver

import scala.collection.immutable._
import scala.collection.mutable.{Map => MutableMap}

import java.util.{LinkedHashMap, ArrayList}
import scala.collection.JavaConversions._

import java.lang.reflect.{Array => ReflectArray, _}

import java.lang.Number

case class TSIncorrectObject(msg: String)
	extends Exception(msg);

/* Universal serialization/deserialization functions
 * 
 * Because Jerkson is responcible for most of the work, all
 * we need to do is to process maps and lists (because Jerkson
 * gives Java's ArrayList and LinkedHashMap.
 * 
 * Also do some work with TSObjects - convert to/from Map them*/
abstract class TSObjectSerdes {
  /*If true, will trace deserialization process on System.out*/
  var trace = false
  
  def doTraceClass(what: String, klass: Class[_]) {
    if(trace)
      System.out.println("SER/DES: " + what + " " + klass)
  }
  
  def doTraceMsg(msg: String) {
    if(trace)
      System.out.println(msg)
  }
  
  def getAbstractClassMap(klass: Class[_], isReverse: Boolean) : (String, Map[String, String]) = {
    val annotation = klass.getAnnotation(classOf[TSObjAbstract])
    var classMap = MutableMap[String, String]() 
    
    if(annotation == null) {
      throw new TSIncorrectObject("Missing TSObjAbstract annotation")
    }
    
    val classMapLength = annotation.classMap.length
    
    if(classMapLength % 2 != 0) {
      throw new TSIncorrectObject("TSObjAbstract.classMap should be pairs of key, className")
    }
    
    for(i <- 0 until classMapLength / 2) {
      val fieldValue = annotation.classMap.apply(i * 2)
      val className = "com.vperflab.tsserver." + annotation.classMap.apply(i * 2 + 1)
      
      if(!isReverse) {
        classMap += fieldValue -> className
      }
      else {
        classMap += className -> fieldValue
      }
    }
    
    return (annotation.fieldName, classMap.toMap)
  }
  
  /* Conversion functions. Because jerkson provides information in LinkedHashMap/ArrayList,
   * try to convert map and list to scala types. If failed, that means we have correct type
   * (List or Map)*/
  def jsonMapToMap(jsonMap: Any) : Map[String, Any] = {
    try {
		var jsonHashMap = jsonMap.asInstanceOf[LinkedHashMap[String, Any]]
		var objMap : Map[String, Any] = mapAsScalaMap(jsonHashMap).toMap
		  
		return objMap
    }
    catch {
      case e: ClassCastException => return jsonMap.asInstanceOf[Map[String, Any]]
    }
  }
  
  def jsonListToList(jsonList: Any) : List[Any] = {
    try {
	    val jsonLinkedList = jsonList.asInstanceOf[ArrayList[Any]]
	    
	    return jsonLinkedList.toList
    }
    catch {
      case e: ClassCastException => return jsonList.asInstanceOf[List[Any]]
    }
  }
  
  def serdesMap(objMap: Map[String, Any], elementClass: Class[_]) : Map[String, Any] = {
    doTraceClass("map of", elementClass)
    var map = MutableMap[String, Any]()
    
    for((key, value) <- objMap) {
      doTraceMsg("KEY: " + key)
      
      map += key -> serdes(value, elementClass)
    }
    
    return map.toMap
  }
  
  def serdesList(objList: List[Any], elementClass: Class[_]) : List[Any] = {
    doTraceClass("list of", elementClass)
    
    var list: List[Any] = Nil
    
    for(value <- objList) {      
      list = list ::: List(serdes(value, elementClass))
    }
    
    return list
  }
  
  def serdesContainer(obj: Any, klass: Class[_], elementClass: Class[_]) : Any = {
    if(klass.isAssignableFrom(classOf[Map[String, Any]])) {
      return serdesMap(jsonMapToMap(obj), elementClass)
    }
    else if(klass.isAssignableFrom(classOf[List[Any]])) {
      return serdesList(jsonListToList(obj), elementClass)
    }
    
    throw new TSIncorrectObject("Invalid container type! Should be immutable.Map or immutable.List, got " + klass)
  }
  
  def serdesField(value: Any, field: Field) : Any = {
    val annotation = field.getAnnotation(classOf[TSObjContainer])
    
    if(annotation != null) {
      return serdesContainer(value, field.getType(), annotation.elementClass)
    }
    
    return serdes(value, field.getType())
  }
  
  def serdesClass(obj: Any, klass: Class[_]) : Any = {    
    if(Modifier.isAbstract(klass.getModifiers())) {
      return serdesAbstractClass(obj, klass)
    }
    
    return serdesInstantiableClass(obj, klass)
  }
  
  def serdesNumber(num: Number, klass: Class[_]): Any =  {
    if(classOf[Double].isAssignableFrom(klass) ||
       classOf[Float].isAssignableFrom(klass)) {
      return num.doubleValue()
    }
    else if(classOf[Long].isAssignableFrom(klass) ||
            classOf[Integer].isAssignableFrom(klass) ||
            classOf[Short].isAssignableFrom(klass) ||
            classOf[Byte].isAssignableFrom(klass)) {
      return num.longValue()
    }

    throw new TSIncorrectObject("Invalid number! Got " + num.getClass() + " instead of " + klass)
  }
  
  /*
   * Serialize/Desirealize object
   * 
   * @param obj - object, which should be ser/des
   * @param klass - desired class
   * 
   * When serializing, obj should be instance of klass, 
   * when deserializing, instance of class will be created from obj
   * */
  def serdes(obj: Any, klass: Class[_]) : Any = {
    if(klass == classOf[AnyRef]) {
      /* If desired class is Any, try to ser/des 
       * according to real type of Any*/
      return serdes(obj, obj.getClass())
    }
    else if(classOf[TSObject].isAssignableFrom(klass)) {
      /* If desired class is derived from TSObject, ser/des it*/
      return serdesClass(obj, klass)
    }
    else if(classOf[Number].isInstance(obj)) {
      return serdesNumber(obj.asInstanceOf[Number], klass)
    }
    else if(klass.isInstance(obj)) {
      /* No need to do ser/des because obj is already have correct class*/
      return obj
    }
     
    throw new TSIncorrectObject("Invalid klass! Got " + obj.getClass() + " instead of " + klass)
  }
  
  def serdesInstantiableClass(obj: Any, klass: Class[_]) : Any
  
  def serdesAbstractClass(obj: Any, klass: Class[_]) : Any
}

object TSObjectDeserializer extends TSObjectSerdes {
  /*Map of loaded classes via classLoader (needed for abstract TSObject)*/
  var loadedClasses = MutableMap[String, Class[_]]()
  
  def serdesInstantiableClass(obj: Any, klass: Class[_]) : Any = {
    val objMap = jsonMapToMap(obj)
    
    doTraceClass("class", klass)
    
	var newObj = klass.newInstance()
	  
	for(field <- klass.getDeclaredFields) {
	  val name = field.getName
	  field.setAccessible(true)
	   
	  doTraceMsg("FIELD: " + name + " of " + field.getType())
	    
	  val value = serdesField(objMap(name), field)
	  
	  field.set(newObj, value)
	}
	  
	return newObj
  }
  
  def classForName(name: String, klass: Class[_]) : Class[_] = {
    try {
      return Class.forName(name)
    }
    catch {
      case e: ClassNotFoundException => {
        if(!(loadedClasses contains name)) {
	      val loader = klass.getClassLoader()
	      val loadedClass = loader.loadClass(name)
	      
	      loadedClasses += name -> loadedClass
	      return loadedClass
	    }
        
        return loadedClasses(name)
      }
    }
  }
  
  def serdesAbstractClass(obj: Any, klass: Class[_]) : Any = {
    val objMap = jsonMapToMap(obj)
    
    doTraceClass("deser abstract class", klass)
    
	val (fieldName, classMap) = getAbstractClassMap(klass, false)
	val classKey = objMap(fieldName).asInstanceOf[String]
	
	val className = classMap(classKey)
    
    doTraceMsg("CLASS: " + classKey + " -> " + className)
    
    var realClass = classForName(className, klass)
    var newObjMap = objMap - fieldName
    
    return serdesInstantiableClass(newObjMap, realClass)
  }
  
  /**
   * Deserialize
   * 
   * @param obj - Map which is generated by Jerkson 
   * @param klass - Desired class to be deserialized
   */
  def doDeserialize(obj: Map[String, Any], klass: Class[_]) : Any = serdes(obj, klass)
}

object TSObjectSerializer extends TSObjectSerdes {
  def serdesInstantiableClass(obj: Any, klass: Class[_]) : Any = {
    doTraceClass("class", klass)
    
	  var objMap = MutableMap[String, Any]()
	  
	  for(field <- klass.getDeclaredFields) {
	    val name = field.getName
	    
	    field.setAccessible(true)
	    objMap += name -> serdesField(field.get(obj), field)
	  }
	  
	  return objMap.toMap
  }
  
  def serdesAbstractClass(obj: Any, klass: Class[_]) : Any = {
    doTraceClass("abstract class", klass)
    
	val (fieldName, classMap) = getAbstractClassMap(klass, true)
	val classKey = obj.getClass().getName()
	val className = classMap(classKey)
    
    doTraceMsg("CLASS: " + classKey + " -> " + className)
    
    val objMap = serdesInstantiableClass(obj, obj.getClass()).asInstanceOf[Map[String, Any]]
    
    return Map(fieldName -> className) ++ objMap 
  }
  
  def doSerialize(obj: TSObject) : Map[String, Any] = {
    return serdes(obj, obj.getClass()).asInstanceOf[Map[String, Any]]
  }
}