Import("env")
import os
import subprocess
import sys
import shutil

def build_web_assets(source, target, env):
    print("=" * 60)
    print("Building web assets with Webpack...")
    print("=" * 60)
    
    project_dir = env.get("PROJECT_DIR")
    frontend_dir = os.path.join(project_dir, "frontend")
    dist_dir = os.path.join(frontend_dir, "dist")
    data_dir = os.path.join(project_dir, "data")
    
    # Ensure frontend dir exists
    if not os.path.exists(frontend_dir):
        print(f"ERROR: Frontend directory not found: {frontend_dir}")
        sys.exit(1)

    # Check for npm
    try:
        subprocess.run(["npm", "--version"], check=True, stdout=subprocess.PIPE)
    except Exception as e:
        print("ERROR: npm not found in PATH.")
        sys.exit(1)
    
    # Clean dist/ and data/ completely
    for path in [dist_dir, data_dir]:
        if os.path.exists(path):
            print(f"Removing old {path}...")
            shutil.rmtree(path)
    
    # Run npm install if needed
    node_modules = os.path.join(frontend_dir, "node_modules")
    if not os.path.exists(node_modules):
        print("Installing npm dependencies...")
        subprocess.run(["npm", "install"], cwd=frontend_dir, check=True)
    
    # Always run webpack build
    print("Running Webpack build...")
    result = subprocess.run(["npm", "run", "build"], cwd=frontend_dir, text=True)
    if result.returncode != 0:
        print("Webpack build failed.")
        sys.exit(result.returncode)

    # Verify dist exists
    if not os.path.exists(dist_dir):
        print("ERROR: dist directory missing after build.")
        sys.exit(1)

    # Copy dist → data
    shutil.copytree(dist_dir, data_dir)
    print("✓ Web assets copied to /data")

    print("=" * 60)
    print("Web assets ready for filesystem build")
    print("=" * 60)

# Always trigger before filesystem image build
env.AddPreAction("$BUILD_DIR/littlefs.bin", build_web_assets)
