"use client"

import { useState, useEffect } from "react"
import { motion } from "framer-motion"
import { useScaleWeight } from "@/hooks/use-scale-weight"
import { Scale, Loader2 } from "lucide-react"
import { WeightGauge } from "./weight-gauge"

export function ScaleVisualization() {
  const { weight, loading, error } = useScaleWeight()
  const [prevWeight, setPrevWeight] = useState(0)
  const [isWeightChanged, setIsWeightChanged] = useState(false)

  // Animation effect when weight changes
  useEffect(() => {
    if (prevWeight !== weight && prevWeight !== 0) {
      setIsWeightChanged(true)
      const timer = setTimeout(() => setIsWeightChanged(false), 1500)
      return () => clearTimeout(timer)
    }
    setPrevWeight(weight)
  }, [weight, prevWeight])

  if (loading) {
    return (
      <div className="flex flex-col items-center justify-center h-full">
        <motion.div
          animate={{ rotate: 360 }}
          transition={{ duration: 2, repeat: Number.POSITIVE_INFINITY, ease: "linear" }}
          className="w-16 h-16 text-blue-400"
        >
          <Loader2 className="w-16 h-16" />
        </motion.div>
        <motion.p
          initial={{ opacity: 0 }}
          animate={{ opacity: 1 }}
          transition={{ delay: 0.5 }}
          className="text-gray-400 mt-4"
        >
          Connecting to scale...
        </motion.p>
      </div>
    )
  }

  if (error) {
    return (
      <div className="flex flex-col items-center justify-center h-full">
        <div className="w-64 h-64 border-4 border-red-600/30 rounded-full flex items-center justify-center bg-red-900/10">
          <div className="text-xl font-bold text-red-400">Error connecting to scale</div>
        </div>
      </div>
    )
  }

  return (
    <div className="flex flex-col items-center justify-center h-full relative">
      <motion.div
        className="relative flex items-center justify-center"
        initial={{ scale: 0.9, opacity: 0 }}
        animate={{ scale: 1, opacity: 1 }}
        transition={{ duration: 0.5 }}
      >
        <WeightGauge weight={weight} />
      </motion.div>

      <motion.div
        className="mt-8 text-gray-400 text-center max-w-xs"
        initial={{ opacity: 0, y: 20 }}
        animate={{ opacity: 1, y: 0 }}
        transition={{ delay: 0.5 }}
      >
        <p className="text-xl text-white">Place food on the scale to measure</p>
      </motion.div>
    </div>
  )
}
