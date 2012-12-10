package com.vperflab.model

import net.liftweb.mapper._

/**
 * @author Dmitry Ivanov (divanov@ambiqtech.ru)
 *         Ambiq Technology Ltd
 */

class Iteration extends LongKeyedMapper[Iteration] with IdPK {

  def getSingleton = Iteration

  object experiment extends MappedLongForeignKey(this, Experiment)

  object duration extends MappedLong(this)

  object clients extends MappedLong(this)

}

object Iteration extends Iteration with LongKeyedMetaMapper[Iteration] {
  override def dbTableName = "iterations"
}
