package com.vperflab.model

import net.liftweb.mapper._


/**
 * @author Dmitry Ivanov (divanov@ambiqtech.ru)
 *         Ambiq Technology Ltd
 */

class Experiment extends LongKeyedMapper[Experiment] with IdPK {

  def getSingleton = Experiment

  object name extends MappedString(this, 128)

  def getIterations: List[Iteration] = Iteration.findAll(By(Iteration.experiment, this))

  def getExecutions: List[Execution] = Execution.findAll(By(Execution.experiment, this))

}

object Experiment extends Experiment with LongKeyedMetaMapper[Experiment] {
  override def dbTableName = "experiments"

  def createAndSave(name: String, iterationDuration: Long, clientVariation: Array[Long]) = {
    val experiment = Experiment.create.name(name)
    experiment.save()

    val iterations = clientVariation.map(
      clientCount => Iteration.create.duration(iterationDuration).clients(clientCount).experiment(experiment)
    )
    iterations.foreach(i => i.save())
    experiment
  }

  override def delete_!(e: Experiment) = {

    e.getExecutions.foreach(Execution.delete_!(_))
    e.getIterations.foreach(Iteration.delete_!(_))

    super.delete_!(e)
  }
}


