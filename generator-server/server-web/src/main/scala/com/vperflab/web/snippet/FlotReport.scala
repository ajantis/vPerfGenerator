package com.vperflab.web.snippet


import scala.xml.NodeSeq
import net.liftweb.util.Helpers._
import net.liftweb.http.js.JsCmds._
import net.liftweb.widgets.flot._
import net.liftweb.common.Full
import net.liftweb.http.js.JE.{JsTrue, JsObj}

/**
 * @author Dmitry Ivanov (divanov@ambiqtech.ru)
 *         Ambiq Technology Ltd
 */

class FlotReport {

  def stackBar(xhtml: NodeSeq, dataValues: List[List[(Double,Double)]], graphAreaId: String = "graph_area") = {

    val data_to_plot = dataValues.map( dataV => new FlotSerie() {
      override val data = dataV
    })

    val options: FlotOptions = new FlotOptions () {


      // NB: barWidth is measured in units of the x-axis, which is unix time
      override val series = Full( Map(
        "stack"-> JsTrue,
        "bars" -> JsObj( "show"->true, "barWidth"-> 10000)
      ) )

      override val xaxis = Full( new FlotAxisOptions() {
        override val mode = Full("time, ms")
      })

      override val legend = Full( new FlotLegendOptions() {
        override val container = Full("value")
      })

    }

    Flot.render(graphAreaId, data_to_plot, options, Flot.script(xhtml))
  }

  def graph(xhtml: NodeSeq, dataValues: List[List[(Double,Double)]], graphAreaId: String = "graph_area") = {

    val data_to_plot = dataValues.map( dataV => new FlotSerie() {
      override val data = dataV
    })

    val options: FlotOptions = new FlotOptions () {

      /*
      // NB: barWidth is measured in units of the x-axis, which is unix time
      override val series = Full( Map(
        "stack"-> JsTrue,
        "bars" -> JsObj( "show"->true, "barWidth"-> 45000000)
      ) )*/

      override val xaxis = Full( new FlotAxisOptions() {
        override val mode = Full("time, ms")
      })

      override val legend = Full( new FlotLegendOptions() {
        override val container = Full("value")
      })

    }

    Flot.render(graphAreaId, data_to_plot, options, Flot.script(xhtml))
  }

  // used as example
  def sine(xhtml: NodeSeq) = {
    val data_values: List[(Double,Double)] = for (i <- List.range (0, 140, 5))
    yield (i / 10.0, Math.sin(i / 10.0) )

    val data_to_plot = new FlotSerie() {
      override val data = data_values
    }

    Flot.render ( "graph_area", List(data_to_plot), new FlotOptions {}, Flot.script(xhtml))
  }
}