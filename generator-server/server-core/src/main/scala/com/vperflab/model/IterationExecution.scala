package com.vperflab.model

import net.liftweb.mapper._

/**
 * @author Dmitry Ivanov (divanov@ambiqtech.ru)
 *         Ambiq Technology Ltd
 */

class IterationExecution extends LongKeyedMapper[IterationExecution] with IdPK {

  def getSingleton = IterationExecution

  object execStartTime extends MappedDateTime(this)

  object execEndTime extends MappedDateTime(this)

  object result extends MappedLong(this)

  object iteration extends MappedLongForeignKey(this, Iteration)

  object execution extends MappedLongForeignKey(this, Execution)

  def execDuration: Long = execEndTime.is.getTime - execStartTime.is.getTime

}

object IterationExecution extends IterationExecution with LongKeyedMetaMapper[IterationExecution] {
  override def dbTableName = "iteration_executions"
}

