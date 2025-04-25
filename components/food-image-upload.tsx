"use client"

import type React from "react"
import { useState, useRef } from "react"
import { Button } from "@/components/ui/button"
import { Card, CardContent } from "@/components/ui/card"
import { Camera, ImageIcon, X, Scan } from "lucide-react"
import { motion, AnimatePresence } from "framer-motion"

interface FoodImageUploadProps {
  onAnalysisComplete: () => void
  onAnalysisStart: () => void
}

export function FoodImageUpload({ onAnalysisComplete, onAnalysisStart }: FoodImageUploadProps) {
  const [selectedImage, setSelectedImage] = useState<File | null>(null)
  const [previewUrl, setPreviewUrl] = useState<string | null>(null)
  const [isUploading, setIsUploading] = useState(false)
  const [error, setError] = useState<string | null>(null)
  const [dragActive, setDragActive] = useState(false)
  const inputRef = useRef<HTMLInputElement>(null)

  const handleImageChange = (e: React.ChangeEvent<HTMLInputElement>) => {
    const file = e.target.files?.[0]
    if (file) {
      setSelectedImage(file)
      setPreviewUrl(URL.createObjectURL(file))
      setError(null)
    }
  }

  const handleDrag = (e: React.DragEvent) => {
    e.preventDefault()
    e.stopPropagation()
    if (e.type === "dragenter" || e.type === "dragover") {
      setDragActive(true)
    } else if (e.type === "dragleave") {
      setDragActive(false)
    }
  }

  const handleDrop = (e: React.DragEvent) => {
    e.preventDefault()
    e.stopPropagation()
    setDragActive(false)

    if (e.dataTransfer.files && e.dataTransfer.files[0]) {
      const file = e.dataTransfer.files[0]
      if (file.type.startsWith("image/")) {
        setSelectedImage(file)
        setPreviewUrl(URL.createObjectURL(file))
        setError(null)
      } else {
        setError("Please select an image file")
      }
    }
  }

  const handleUpload = async () => {
    if (!selectedImage) {
      setError("Please select an image first")
      return
    }

    setIsUploading(true)
    onAnalysisStart()

    try {
      const formData = new FormData()
      formData.append("image", selectedImage)

      const response = await fetch("/api/analyze-food", {
        method: "POST",
        body: formData,
      })

      if (!response.ok) {
        throw new Error("Failed to analyze image")
      }

      // We don't need to handle the response data here anymore
      // as it's being stored in Firebase and will be fetched via the hook
      onAnalysisComplete()
    } catch (error) {
      console.error("Error uploading image:", error)
      setError("Failed to analyze image. Please try again.")
      onAnalysisComplete()
    } finally {
      setIsUploading(false)
    }
  }

  const clearImage = () => {
    setSelectedImage(null)
    setPreviewUrl(null)
    if (inputRef.current) {
      inputRef.current.value = ""
    }
  }

  return (
    <motion.div initial={{ opacity: 0, y: 20 }} animate={{ opacity: 1, y: 0 }} transition={{ duration: 0.5 }}>
      <Card className="w-full bg-gray-800 border-gray-700 shadow-xl overflow-hidden">
        <CardContent className="p-4">
          <div className="flex flex-col items-center space-y-4">
            <div
              className={`relative border-2 ${dragActive ? "border-blue-500 bg-blue-500/10" : "border-dashed border-gray-600"} 
                rounded-xl p-4 w-full h-56 flex flex-col items-center justify-center cursor-pointer 
                hover:border-blue-500 hover:bg-blue-500/5 transition-all duration-300`}
              onClick={() => inputRef.current?.click()}
              onDragEnter={handleDrag}
              onDragLeave={handleDrag}
              onDragOver={handleDrag}
              onDrop={handleDrop}
            >
              <input
                ref={inputRef}
                id="food-image-input"
                type="file"
                accept="image/*"
                className="hidden"
                onChange={handleImageChange}
              />

              <AnimatePresence mode="wait">
                {previewUrl ? (
                  <motion.div
                    key="preview"
                    initial={{ opacity: 0 }}
                    animate={{ opacity: 1 }}
                    exit={{ opacity: 0 }}
                    className="relative w-full h-full flex items-center justify-center"
                  >
                    <img
                      src={previewUrl || "/placeholder.svg"}
                      alt="Food preview"
                      className="max-h-full max-w-full object-contain rounded-lg"
                    />
                    <motion.button
                      whileHover={{ scale: 1.1 }}
                      whileTap={{ scale: 0.9 }}
                      className="absolute top-2 right-2 bg-gray-800/80 p-1.5 rounded-full text-gray-300 hover:text-white"
                      onClick={(e) => {
                        e.stopPropagation()
                        clearImage()
                      }}
                    >
                      <X size={16} />
                    </motion.button>
                  </motion.div>
                ) : (
                  <motion.div
                    key="upload"
                    initial={{ opacity: 0 }}
                    animate={{ opacity: 1 }}
                    exit={{ opacity: 0 }}
                    className="flex flex-col items-center text-gray-400"
                  >
                    <div className="relative">
                      <motion.div
                        animate={{
                          y: [0, -5, 0],
                        }}
                        transition={{
                          repeat: Number.POSITIVE_INFINITY,
                          duration: 2,
                          repeatType: "reverse",
                        }}
                      >
                        <div className="bg-blue-500/20 p-4 rounded-full mb-3">
                          <Camera className="h-10 w-10 text-blue-400" />
                        </div>
                      </motion.div>
                      <motion.div
                        className="absolute -right-2 -top-2 bg-purple-500/20 p-2 rounded-full"
                        animate={{
                          scale: [1, 1.1, 1],
                        }}
                        transition={{
                          repeat: Number.POSITIVE_INFINITY,
                          duration: 2,
                          repeatType: "reverse",
                          delay: 0.5,
                        }}
                      >
                        <ImageIcon className="h-4 w-4 text-purple-400" />
                      </motion.div>
                    </div>

                    <p className="text-lg font-medium mb-1">Upload Food Image</p>
                    <p className="text-gray-500 text-sm text-center">Drag & drop or click to select</p>
                  </motion.div>
                )}
              </AnimatePresence>
            </div>

            {error && (
              <motion.p initial={{ opacity: 0, y: 5 }} animate={{ opacity: 1, y: 0 }} className="text-red-400 text-sm">
                {error}
              </motion.p>
            )}

            <motion.div
              className="w-full"
              initial={{ opacity: 0, y: 10 }}
              animate={{ opacity: 1, y: 0 }}
              transition={{ delay: 0.2 }}
            >
              <Button
                onClick={handleUpload}
                disabled={!selectedImage || isUploading}
                className="w-full bg-blue-600 hover:bg-blue-700 text-white border-0 h-12 font-medium"
              >
                {isUploading ? (
                  <motion.span className="flex items-center" initial={{ opacity: 0 }} animate={{ opacity: 1 }}>
                    <svg
                      className="animate-spin -ml-1 mr-2 h-5 w-5 text-white"
                      xmlns="http://www.w3.org/2000/svg"
                      fill="none"
                      viewBox="0 0 24 24"
                    >
                      <circle
                        className="opacity-25"
                        cx="12"
                        cy="12"
                        r="10"
                        stroke="currentColor"
                        strokeWidth="4"
                      ></circle>
                      <path
                        className="opacity-75"
                        fill="currentColor"
                        d="M4 12a8 8 0 018-8V0C5.373 0 0 5.373 0 12h4zm2 5.291A7.962 7.962 0 014 12H0c0 3.042 1.135 5.824 3 7.938l3-2.647z"
                      ></path>
                    </svg>
                    <span>Analyzing Food...</span>
                  </motion.span>
                ) : (
                  <motion.span
                    className="flex items-center"
                    whileHover={{ x: 5 }}
                    transition={{ type: "spring", stiffness: 400 }}
                  >
                    <Scan className="mr-2 h-5 w-5" />
                    <span>Analyze Food</span>
                  </motion.span>
                )}
              </Button>
            </motion.div>
          </div>
        </CardContent>
      </Card>
    </motion.div>
  )
}
