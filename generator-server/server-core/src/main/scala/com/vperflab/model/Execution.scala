package com.vperflab.model

import net.liftweb.mapper._

/**
 * @author Dmitry Ivanov
 */

class Execution extends LongKeyedMapper[Execution] with IdPK {

  def getSingleton = Execution

  object experiment extends MappedLongForeignKey(this, Experiment)

  object execStartTime extends MappedDateTime(this)

  object duration extends MappedLong(this)

  def getIterationExecutions: List[IterationExecution] = IterationExecution.findAll(By(IterationExecution.execution, this)).sortWith( (i1,i2) => i1.execStartTime.is.before(i2.execStartTime.is))

}

object Execution extends Execution with LongKeyedMetaMapper[Execution] {
  override def dbTableName = "executions"

  override def delete_!(e: Execution) = {

    e.getIterationExecutions.foreach(IterationExecution.delete_!(_))

    super.delete_!(e)
  }

}
