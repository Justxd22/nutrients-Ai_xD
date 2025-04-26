"use client"

import { useEffect, useRef } from "react"

interface WeightGaugeProps {
  weight: number
  maxWeight?: number
}

export function WeightGauge({ weight, maxWeight = 500 }: WeightGaugeProps) {
  const canvasRef = useRef<HTMLCanvasElement>(null)
  const gaugeRef = useRef<any>(null) // `any` because we import it dynamically

  useEffect(() => {
    if (!canvasRef.current) return

    let mounted = true

    import("canvas-gauges").then(({ RadialGauge }) => {
      if (!mounted || !canvasRef.current) return

      if (!gaugeRef.current) {
        gaugeRef.current = new RadialGauge({
          renderTo: canvasRef.current,
          width: 300,
          height: 300,
          units: "grams",
          title: "Weight",
          minValue: 0,
          maxValue: maxWeight,
          majorTicks: [0, 50, 100, 150, 200, 250, 300, 350, 400, 450, 500],
          minorTicks: 2,
          strokeTicks: true,
          highlights: [
            { from: 0, to: 150, color: "rgba(159, 159, 159, 0.3)" },
            { from: 150, to: 250, color: "rgba(203, 203, 203, 0.45)" },
            { from: 250, to: 350, color: "rgba(219, 219, 219, 0.45)" },
            { from: 350, to: 500, color: "rgba(234, 234, 234, 0.45)" },
          ],
          colorPlate: "#222",
          colorMajorTicks: "#f5f5f5",
          colorMinorTicks: "#ddd",
          colorTitle: "#fff",
          colorUnits: "#ccc",
          colorNumbers: "#eee",
          colorNeedle: "#f63b3e",
          colorNeedleEnd: "#f5f5f5",
          valueBox: true,
          animationRule: "linear",
          animationDuration: 500,
          fontNumbersSize: 20,
          fontTitleSize: 24,
          fontUnitsSize: 22,
          fontValueSize: 35,
          fontValue: "Orbitron",
          fontNumbers: "Orbitron",
          borders: true,
          borderOuterWidth: 0,
          borderMiddleWidth: 0,
          borderInnerWidth: 0,
          colorBorderOuter: "#333",
          colorBorderMiddle: "#222",
          colorBorderInner: "#111",
          colorValueBoxBackground: "#b0cfb4",
          needleType: "arrow",
          needleWidth: 3,
          needleCircleSize: 7,
          needleCircleOuter: true,
          needleCircleInner: true,
          animatedValue: true,
        })
        gaugeRef.current.draw()
      }

      // Update the gauge value
      gaugeRef.current.value = weight
      gaugeRef.current.update()
    })

    return () => {
      mounted = false
      gaugeRef.current = null
    }
  }, [weight, maxWeight])

  return <canvas ref={canvasRef} />
}
